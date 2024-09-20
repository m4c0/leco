#pragma leco tool
#include "sim.hpp"
#include "targets.hpp"

#include <stdio.h>
#include <string.h>

import gopt;
import mtime;
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
  sim_sbt stem {};
  sim_sb_path_copy_stem(&stem, fw);
  
  sim_sbt path {};
  sim_sb_path_copy_parent(&path, fw);

  fprintf(out, "-F%s\n-framework\n%s\n", path.buffer, stem.buffer);
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
  sim_sbt obj{};
  sim_sb_copy(&obj, dag);
  sim_sb_path_set_extension(&obj, "o");
  put(obj.buffer);

  mtime = max(mtime, mtime::of(obj.buffer));
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

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, input);
  target = sim_sb_path_filename(&path);

  sim_sbt args{};
  sim_sb_copy(&args, input);
  sim_sb_path_set_extension(&args, "link");

  // TODO: move argument build (and mtime check) somewhere else
  //       just in case we want to force a link for any reason
  out = sys::fopen(args.buffer, "wb");
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

  remove(next.buffer);
  remove(prev.buffer);
#endif

  sim_sbt cmd{10240};
  sim_sb_path_copy_parent(&cmd, argv[0]);
  sim_sb_path_append(&cmd, "leco-clang.exe");
  sim_sb_printf(&cmd, " -t %s", target);
  if (debug)
    sim_sb_concat(&cmd, " -g");
  if (opt)
    sim_sb_concat(&cmd, " -O");
  sim_sb_printf(&cmd, " -- @%s -o ", args.buffer);
#ifdef _WIN32
  sim_sb_concat(&cmd, next.buffer);
  // otherwise, face LNK1107 errors from MSVC
  sim_sb_concat(&cmd, " -fuse-ld=lld");
#else
  sim_sb_concat(&cmd, output);
#endif

  if (IS_TGT(target, TGT_OSX)) {
    sim_sb_concat(&cmd, " -rpath @executable_path");
  } else if (IS_TGT_IOS(target)) {
    sim_sb_concat(&cmd, " -rpath @executable_path/Frameworks");
  } else if (IS_TGT(target, TGT_WINDOWS)) {
    sim_sbt rc{};
    sim_sb_copy(&rc, input);
    sim_sb_path_set_extension(&rc, "res");
    if (mtime::of(rc.buffer) > 0) {
      sim_sb_printf(&cmd, " %s", rc.buffer);
    }
  } else if (IS_TGT(target, TGT_WASM)) {
    sim_sbt sra{};

    auto f = sys::fopen("../leco/out/wasm32-wasi/sysroot", "r");
    fgets(sra.buffer, sra.size, f);
    fclose(f);

    // export-table: allows passing lambdas to JS
    sim_sb_concat(&cmd, " -Xlinker --export-table");
    // exec-model: without it, we could use "main" but _start calls the global
    //             dtor after "main" returns
    sim_sb_concat(&cmd, " -mexec-model=reactor");
    // no-check-features: allows using shared-memory without atomics/etc
    //                    https://stackoverflow.com/a/70186219
    //                    Only works because we won't be using malloc in threads
    // sim_sb_concat(&cmd, " -Xlinker --shared-memory -Xlinker --no-check-features");

    sim_sb_printf(&cmd, " -resource-dir %s", sra.buffer);
  }

  sys::log("linking", output);
  sys::run(cmd.buffer);

#ifdef _WIN32
  rename(output, prev.buffer);
  rename(next.buffer, output);
#endif

  return 0;
} catch (...) {
  fprintf(stderr, "err\n");
  return 1;
}
