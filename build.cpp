#define MCT_STAT_IMPLEMENTATION
#define SYSSTD_IMPLEMENTATION
#include "targets.hpp"
#include "../mct/mct-stat.h"
#include "../sysstd/sysstd.h"

#include <stdio.h>
#include <stdlib.h>

#define SEP SYSSTD_FILE_SEP

#define MKOUT(name) \
  sysstd_mkdir(".." SEP name SEP "out"); \
  sysstd_mkdir(".." SEP name SEP "out" SEP HOST_TARGET);

#define CLANG "out" SEP HOST_TARGET SEP "leco-clang.exe"

#define CPPSTD " -std=c++2b"
#define PCMFL " --precompile" CPPSTD
#define PMP " -fprebuilt-module-path=out" SEP HOST_TARGET

#define PCM(name) ".." SEP name SEP "out" SEP HOST_TARGET SEP name ".pcm"
#define LPCM(name) "out" SEP HOST_TARGET SEP name ".pcm"
#define PARG(name) " -fmodule-file=" name "=" PCM(name)
#define MARG(name) " -fmodule-file=" name "=" PCM(name) " " PCM(name)
#define LMARG(name) " -fmodule-file=" name "=" LPCM(name) " " LPCM(name)

#define MODULE(name, ...) MKOUT(name); run(CLANG " -i .." SEP name SEP name ".cppm -- -o " PCM(name) PCMFL __VA_ARGS__);
#define LOCAL_MODULE(name, ...) run(CLANG " -i " name ".cppm -- -o " LPCM(name) PCMFL PMP __VA_ARGS__);

#define TOOL(name)                                                    \
  puts("Building " name);                                             \
  run(CLANG " -i leco-" name ".cpp "                                  \
            "-- -o out/" HOST_TARGET "/leco-" name ".exe " CPPSTD     \
            MARG("mtime") MARG("no") MARG("popen")                    \
            MARG("pprent") MARG("print") MARG("sysstd")               \
            LMARG("sim") LMARG("strset") LMARG("sys"))

static void run(const char * cmd) {
  if (0 == system(cmd)) return;
  fprintf(stderr, "command failed: %s\n", cmd);
  throw 0;
}

int try_main() {
  MKOUT("leco");

  puts("Building clang runner");
  run("clang++ -std=c++20 leco-clang.cpp -o " CLANG);

  puts("Building core modules");
  MODULE("mtime");
  MODULE("no");
  MODULE("popen", PARG("no"));
  MODULE("pprent");
  MODULE("print");
  MODULE("sysstd");
  LOCAL_MODULE("sim");
  LOCAL_MODULE("strset");
  LOCAL_MODULE("sys", PARG("mtime") PARG("no") PARG("popen") PARG("print") PARG("pprent") PARG("sysstd"));

  TOOL("dagger");
  TOOL("deplist");
  TOOL("link");
  TOOL("obj");
  TOOL("pcm");
  TOOL("pcm2obj");

  TOOL("driver");
  TOOL("meta");

  puts("Building meta runner");
  run(CLANG " -i leco.cpp -- -o leco.exe");

  puts("Using LECO to build final stage");
  run("." SEP "leco.exe");

  puts("Doney-devito");
  return 0;
}

int main() try {
  if (mct_stat_mtime("build.cpp") > mct_stat_mtime("build.exe")) {
    puts("Rebuilding self");
    remove("build.old");
    run("clang++ -std=c++20 build.cpp -o build.new");
    rename("build.exe", "build.old");
    rename("build.new", "build.exe");
    run("." SEP "build.exe");
    return 0;
  }
  return try_main();
} catch (...) {
  return 1;
}
