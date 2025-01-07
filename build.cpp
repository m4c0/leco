#define MTIME_IMPLEMENTATION
#define SYSSTD_IMPLEMENTATION
#include "targets.hpp"
#include "../mtime/mtime.h"
#include "../sysstd/sysstd.h"

#include <stdio.h>
#include <stdlib.h>

#define SEP SYSSTD_FILE_SEP

#define CLANG "out" SEP HOST_TARGET SEP "leco-clang.exe"

#define PCM(name) ".." SEP name SEP "out" SEP HOST_TARGET SEP name ".pcm"
#define LPCM(name) "out" SEP HOST_TARGET SEP name ".pcm"
#define MARG(name) " -fmodule-file=" name "=" PCM(name) " " PCM(name)
#define LMARG(name) " -fmodule-file=" name "=" LPCM(name) " " LPCM(name)

#define MODULE(name) run(CLANG " -i .." SEP name SEP name ".cppm");
#define LOCAL_MODULE(name) run(CLANG " -i " name ".cppm");

#define TOOL(name)                                                             \
  puts("Building " name);                                                      \
  run(CLANG " -i leco-" name ".cpp -o out/" HOST_TARGET "/leco-" name ".exe "  \
            "--" MARG("gopt") MARG("mtime") MARG("popen") MARG("pprent")       \
                 MARG("print") MARG("sysstd") \
                LMARG("sim") LMARG("strset") LMARG("sys"))

static void run(const char * cmd) {
  if (0 == system(cmd)) return;
  fprintf(stderr, "command failed: %s\n", cmd);
  throw 0;
}

int try_main(int argc, char **argv) {
  sysstd_mkdir("out");
  sysstd_mkdir("out" SEP HOST_TARGET);

  puts("Building clang runner");
  run("clang++ -Wall -Wno-unknown-pragmas -std=c++20 leco-clang.cpp -o " CLANG);

  puts("Building core modules");
  MODULE("gopt");
  MODULE("mtime");
  MODULE("popen");
  MODULE("pprent");
  MODULE("print");
  MODULE("sysstd");
  LOCAL_MODULE("sim");
  LOCAL_MODULE("strset");
  LOCAL_MODULE("sys");

  TOOL("dagger");
  TOOL("deplist");
  TOOL("link");
  TOOL("obj");
  TOOL("pcm");

  TOOL("driver");
  TOOL("meta");

  puts("Building meta runner");
  run(CLANG " -i leco.cpp -o leco.exe");

  puts("Using LECO to build final stage");
  run("." SEP "leco.exe");

  puts("Doney-devito");
  return 0;
}

int main(int argc, char **argv) try {
  if (mtime_of("build.cpp") > mtime_of("build.exe")) {
    puts("Rebuilding self");
    remove("build.old");
    remove("build.new");
    run("clang++ -std=c++20 build.cpp -o build.new");
    rename("build.exe", "build.old");
    rename("build.new", "build.exe");
    run("." SEP "build.exe");
    return 0;
  }
  return try_main(argc, argv);
} catch (...) {
  return 1;
}
