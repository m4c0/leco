#pragma leco tool
#include "targets.hpp"

#include <stdio.h>
#include <string.h>

import gopt;
import mtime;
import sim;
import strset;
import sys;

static const char *target{};
static FILE *out{};
static const char *argv0{};

static void usage() {
  sys::die(R"(
Usage: %s -i <input.dag> -o <output.exe> [-g] [-O]

Where:
        -i: input DAG
        -o: output executable
        -g: enable debug symbols
        -O: enable optimisations
)",
      argv0);
}

static void put(const char *a) {
  while (*a != 0) {
    char c = *a++;
    if (c == '\\') {
      fputs("\\\\", out); // escapes backslash
    } else {
      fputc(c, out);
    }
  }
  fputc('\n', out);
}

static constexpr auto max(auto a, auto b) { return a > b ? a : b; }

static void add_local_fw(const char * fw) {
  auto stem = sim::path_stem(fw);
  auto path = sim::path_parent(fw);

  fprintf(out, "-F%s\n-framework\n%s\n", *path, stem.buffer);
}

static str::map cache{};
static auto read_dag(const char *dag) {
  auto & mtime = cache[dag];
  if (mtime != 0) return mtime;
  mtime = 1;

  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
    case 'tdll':
      fprintf(out, "-shared\n");
      break;
    case 'frwk':
      fprintf(out, "-framework\n%s\n", file);
      break;
    case 'libr':
      fprintf(out, "-l%s\n", file);
      break;
    case 'ldir':
      fprintf(out, "-L%s\n", file);
      break;
    case 'slib':
      fprintf(out, "%s\n", file);
      break;
    case 'xcfw': add_local_fw(file); break;
    case 'idag':
    case 'mdag': mtime = max(mtime, read_dag(file)); break;
    default: break;
    }
  });

  // Add object after dependencies as this kinda enforces the static init order
  sim::sb obj { dag };
  obj.path_extension("o");
  put(*obj);

  mtime = max(mtime, mtime::of(*obj));
  return mtime;
}

int main(int argc, char **argv) try {
  argv0 = argv[0];

  const char *input{};
  const char *output{};
  bool debug{};
  bool opt{};
  auto opts = gopt_parse(argc, argv, "i:o:gO", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    case 'o':
      output = val;
      break;
    case 'g':
      debug = true;
      break;
    case 'O':
      opt = true;
      break;
    default:
      usage();
    }
  });
  if (!input || !output)
    usage();

  auto path = sim::path_parent(input);
  target = path.path_filename();

  sim::sb args { input };
  args.path_extension("link");

  // TODO: move argument build (and mtime check) somewhere else
  //       just in case we want to force a link for any reason
  out = sys::fopen(*args, "wb");
  for (auto i = 0; i < opts.argc; i++) {
    fprintf(out, "%s\n", opts.argv[i]);
  }
  auto mtime = read_dag(input);
  fclose(out);

  if (mtime <= mtime::of(output)) return 0;

#ifdef _WIN32
  // We can rename but we can't overwrite an open executable.
  // To link "clang" we need the old version, so we have to compile in a new
  // place then do the appropriate renames.

  sim_sbt next{};
  sim_sb_copy(&next, output);
  sim_sb_concat(&next, ".new");

  sim_sbt prev{};
  sim_sb_copy(&prev, output);
  sim_sb_concat(&prev, ".old");

  remove(*next);
  remove(*prev);
#endif

  sim::sb cmd{10240};
  cmd.printf(" -t %s", target);
  if (debug) cmd += " -g";
  if (opt) cmd += " -O";
  cmd.printf(" -- @%s -o ", *args);
#ifdef _WIN32
  cmd += *next;
  // otherwise, face LNK1107 errors from MSVC
  cmd += " -fuse-ld=lld";
#else
  cmd += output;
#endif

  if (IS_TGT(target, TGT_OSX)) {
    cmd += " -rpath @executable_path/../Frameworks";
  } else if (IS_TGT_IOS(target)) {
    cmd += " -rpath @executable_path/Frameworks";
  } else if (IS_TGT(target, TGT_WINDOWS)) {
    sim::sb rc { input };
    rc.path_extension("res");
    if (mtime::of(*rc) > 0) cmd.printf(" %s", *rc);
  } else if (IS_TGT(target, TGT_WASM)) {
    char sra[1024] {};

    auto f = sys::fopen("../leco/out/wasm32-wasi/sysroot", "r");
    fgets(sra, sizeof(sra) - 1, f);
    fclose(f);

    cmd.printf(" -resource-dir %s", sra);

    // export-table: allows passing lambdas to JS
    cmd += " -Xlinker --export-table";
    // exec-model: without it, we could use "main" but _start calls the global
    //             dtor after "main" returns
    cmd += " -mexec-model=reactor";
    // no-check-features: allows using shared-memory without atomics/etc
    //                    https://stackoverflow.com/a/70186219
    //                    Only works because we won't be using malloc in threads
    // cmd += " -Xlinker --shared-memory -Xlinker --no-check-features";
  }

  sys::log("linking", output);
  sys::tool_run("clang", "%s", *cmd);

#ifdef _WIN32
  rename(output, *prev);
  rename(*next, output);
#endif

  return 0;
} catch (...) {
  fprintf(stderr, "err\n");
  return 1;
}
