#define SIM_IMPLEMENTATION
#include "die.hpp"
#include "mkdir.h"
#include "targets.hpp"

#include <stdio.h>

#if _WIN32
#define SEP "\\"
#else
#define SEP "/"
#endif

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
                LMARG("strset") LMARG("sys"))

int try_main(int argc, char **argv) {
  // TODO: self-rebuild this cpp
  mkdirs("out" SEP HOST_TARGET);

  puts("Building clang runner");
  run("clang++ -Wall -Wno-unknown-pragmas -std=c++20 leco-clang.cpp -o " CLANG);

  puts("Building core modules");
  MODULE("gopt");
  MODULE("mtime");
  MODULE("popen");
  MODULE("pprent");
  LOCAL_MODULE("strset");
  LOCAL_MODULE("sys");

  puts("Building meta runner");
  run(CLANG " -i leco.cpp -o leco.exe");

  TOOL("meta");
  TOOL("deplist");
  TOOL("dagger");
  TOOL("link");
  TOOL("recurse");
  TOOL("driver");

  puts("Using LECO to build final stage");
  run("." SEP "leco.exe");

  puts("Doney-devito");
  return 0;
}

int main(int argc, char **argv) try {
  return try_main(argc, argv);
} catch (int n) {
  fprintf(stderr, "child process failed with code %d\n", n);
}
