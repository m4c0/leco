#pragma leco tool

#include <stdio.h>

import sys;

static void put_escape(FILE * out, const char *a) {
  while (*a != 0) {
    char c = *a++;
    if (c == '\\') fputs("\\\\", out); // escapes backslash
    else fputc(c, out);
  }
  fputc('\n', out);
}

static void add_local_fw(FILE * out, const char * fw) {
  auto stem = sim::path_stem(fw);
  auto path = sim::path_parent(fw);

  fprintf(out, "-F%s\n-framework\n%s\n", *path, stem.buffer);
}

static auto read_dag(str::map & cache, const char * dag, FILE * out) {
  auto & mtime = cache[dag];
  if (mtime != 0) return mtime;
  mtime = 1;

  sim::sb obj {};
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
    case 'tdll': fprintf(out, "-shared\n"); break;
    case 'frwk': fprintf(out, "-framework\n%s\n", file); break;
    case 'libr': fprintf(out, "-l%s\n", file); break;
    case 'ldir': fprintf(out, "-L%s\n", file); break;
    case 'rpth': fprintf(out, "-Wl,-rpath,%s\n", file); break;
    case 'slib': fprintf(out, "%s\n", file); break;
    case 'objf': obj = sim::sb { file }; break;
    case 'xcfw': add_local_fw(out, file); break;
    case 'idag':
    case 'mdag': mtime = sys::max(mtime, read_dag(cache, file, out)); break;
    default: break;
    }
  });

  // Add object after dependencies as this kinda enforces the static init order
  put_escape(out, *obj);

  mtime = sys::max(mtime, mtime::of(*obj));
  return mtime;
}

void run(const char * input, const char * output) {
  sim::sb args { input };
  args.path_extension("link");

  // TODO: move argument build (and mtime check) somewhere else
  //       just in case we want to force a link for any reason
  auto out = sys::fopen(*args, "wb");
  str::map cache {};
  auto mtime = read_dag(cache, input, out);
  fclose(out);

  if (mtime <= mtime::of(output)) return;

#ifdef _WIN32
  // We can rename but we can't overwrite an open executable.
  // To link "clang" we need the old version, so we have to compile in a new
  // place then do the appropriate renames.

  auto next = sim::sb { output } + ".new";
  auto prev = sim::sb { output } + ".old";

  remove(*next);
  remove(*prev);
#endif

  sim::sb cmd{10240};
  cmd.printf(" -- @%s -o ", *args);
#ifdef _WIN32
  cmd += *next;
  // otherwise, face LNK1107 errors from MSVC
  cmd += " -fuse-ld=lld";
#else
  cmd += output;
#endif

  if (sys::is_tgt_osx()) {
    // Required for custom frameworks
    cmd += " -rpath @executable_path/../Frameworks";
    // Useful for third-party dylibs, like vulkan loader
    cmd += " -rpath @executable_path";
  } else if (sys::is_tgt_ios()) {
    cmd += " -rpath @executable_path/Frameworks";
  } else if (sys::is_tgt_windows()) {
    auto rc = sim::sb { input }.path_extension("res");
    if (mtime::of(*rc) > 0) cmd.printf(" %s", *rc);
  } else if (sys::is_tgt_wasm()) {
    char sra[1024] {};

    fgets(sra, sizeof(sra) - 1, sys::file { "../leco/out/wasm32-wasi/sysroot", "r" });

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
}

int main() try {
  sys::for_each_root_dag([](auto * dag, auto id, auto file) {
    if (id != 'tmmd') run(dag, file);
  });

  return 0;
} catch (...) {
  return (fprintf(stderr, "err\n"), 1);
}
