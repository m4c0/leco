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

void run(const char *cmd) {
  auto res = system(cmd);
  if (res != 0)
    throw res;
}
int try_main(int argc, char **argv) {
  // TODO: self-rebuild "phase 0" (aka "this cpp")

  puts("Building clang runner");
  run("clang++ -Wall -Wno-unknown-pragmas -std=c++20 leco-clang.cpp "
      "-o leco-clang.exe");

  puts("Building deplist");
  run("." SEP "leco-clang.exe -i leco-deplist.cpp -o leco-deplist.exe");

  puts("Building dagger");
  run("." SEP "leco-clang.exe -i leco-dagger.cpp -o leco-dagger.exe");

  puts("Building linker");
  run("." SEP "leco-clang.exe -i leco-link.cpp -o leco-link.exe");

  // TODO: move "evoker.cpp" bits to "clang.cpp"
  // TODO: make phase1 leaner
  puts("Building Phase 1");
  run("." SEP "leco-clang.exe -- "
      "bouncer.cpp dag.cpp dag_plugin.cpp "
      "evoker.cpp impls.cpp leco.cpp "
      "phase1.cpp -o phase1.exe");

  puts("Using Phase 1 to build final stage");
  run("." SEP "phase1.exe");

  puts("Moving final stage to root folder");
  remove("leco.exe");
  if (0 != rename("out/" HOST_TARGET "/leco.exe", "leco.exe")) {
    perror("failed to rename");
    return 1;
  }

  puts("Doney-devito");
  return 0;
}

int main(int argc, char **argv) try {
  return try_main(argc, argv);
} catch (int n) {
  fprintf(stderr, "child process failed with code %d\n", n);
}
