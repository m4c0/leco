#define SIM_IMPLEMENTATION
#include "die.hpp"
#include "mkdir.h"
#include "targets.hpp"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
#define SEP "\\"
#else
#define SEP "/"
#endif

#define CLANG "out" SEP HOST_TARGET SEP "leco-clang.exe"

#define GOPT_PCM "../gopt/out/" HOST_TARGET "/gopt.pcm"
#define GOPT                                                                   \
  "-fmodule-file=gopt=" GOPT_PCM " ../gopt/out/" HOST_TARGET "/gopt.o"

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

  puts("Building deplist");
  run(CLANG " -i leco-deplist.cpp -o out/" HOST_TARGET "/leco-deplist.exe "
            "-- " GOPT);

  puts("Building dagger");
  run(CLANG " -i leco-dagger.cpp -o out/" HOST_TARGET "/leco-dagger.exe "
            "-- " GOPT);

  puts("Building linker");
  run(CLANG " -i leco-link.cpp -o out/" HOST_TARGET "/leco-link.exe "
            "-- " GOPT);

  puts("Building recurser");
  run(CLANG " -i leco-recurse.cpp -o out/" HOST_TARGET "/leco-recurse.exe "
            "-- " GOPT);

  puts("Building driver");
  run(CLANG " -i leco-driver.cpp -o out/" HOST_TARGET "/leco-driver.exe "
            "-- " GOPT);

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
