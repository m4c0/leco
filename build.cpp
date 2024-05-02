#include "host_target.hpp"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
#define SEP "\\"
#else
#define SEP "/"
#endif

int main(int argc, char **argv) {
  // TODO: self-rebuild "phase 0" (aka "this cpp")

  puts("Building clang runner");
  if (0 != system("clang++ -std=c++20 clang.cpp -o leco-clang.exe"))
    return 1;

  puts("Building dagger");
  if (0 != system("." SEP "leco-clang.exe -- "
                  "dagger.cpp -o leco-dagger.exe"))
    return 1;

  // TODO: move "evoker.cpp" bits to "clang.cpp"
  // TODO: make "compile.cpp" a top-level unit
  // TODO: make phase1 leaner
  puts("Building Phase 1");
  if (0 != system("." SEP "leco-clang.exe -- "
                  "bouncer.cpp compile.cpp "
                  "dag.cpp dag_plugin.cpp evoker.cpp "
                  "impls.cpp leco.cpp "
                  "phase1.cpp -o phase1.exe"))
    return 1;

  puts("Using Phase 1 to build final stage");
  if (0 != system("." SEP "phase1.exe")) {
    return 1;
  }

  puts("Moving final stage to root folder");
  remove("leco.exe");
  if (0 != rename("out/" HOST_TARGET "/leco.exe", "leco.exe")) {
    perror("failed to rename");
    return 1;
  }

  puts("Doney-devito");
  return 0;
}
