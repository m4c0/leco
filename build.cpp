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

#define GOPT_PCM "../gopt/out/" HOST_TARGET "/gopt.pcm"
#define GOPT                                                                   \
  "-fmodule-file=gopt=" GOPT_PCM " ../gopt/out/" HOST_TARGET "/gopt.o"

#define TOOL(name)                                                             \
  puts("Building " name);                                                      \
  run(CLANG " -i leco-" name ".cpp -o out/" HOST_TARGET "/leco-" name ".exe "  \
            "-- " GOPT)

int try_main(int argc, char **argv) {
  // TODO: self-rebuild this cpp
  mkdirs("out" SEP HOST_TARGET);

  puts("Building clang runner");
  run("clang++ -Wall -Wno-unknown-pragmas -std=c++20 leco-clang.cpp -o " CLANG);

  puts("Building core modules");
  run(CLANG " -i ../gopt/gopt.cppm");
  run(CLANG " -i " GOPT_PCM);

  puts("Building meta runner");
  run(CLANG " -i leco.cpp -o leco.exe");

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
