#define MKDIR_IMPLEMENTATION
#define SIM_IMPLEMENTATION
#include "host_target.hpp"
#include "mkdir.h"

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

void run(const char *cmd) {
  auto res = system(cmd);
  if (res != 0)
    throw res;
}
int try_main(int argc, char **argv) {
  // TODO: self-rebuild "phase 0" (aka "this cpp")
  mkdirs("out" SEP HOST_TARGET);

  puts("Building meta runner");
  run("clang++ -Wall -Wno-unknown-pragmas -std=c++20 leco.cpp -o leco.exe");

  puts("Building clang runner");
  run("clang++ -Wall -Wno-unknown-pragmas -std=c++20 leco-clang.cpp -o " CLANG);

  puts("Building deplist");
  run(CLANG " -i leco-deplist.cpp -o out/" HOST_TARGET "/leco-deplist.exe");

  puts("Building dagger");
  run(CLANG " -i leco-dagger.cpp -o out/" HOST_TARGET "/leco-dagger.exe");

  puts("Building sysroot finder");
  run(CLANG " -i leco-sysroot.cpp -o out/" HOST_TARGET "/leco-sysroot.exe");

  puts("Building linker");
  run(CLANG " -i leco-link.cpp -o out/" HOST_TARGET "/leco-link.exe");

  // TODO: make phase1 leaner
  puts("Building Phase 1");
  run(CLANG " -- bouncer.cpp dag.cpp dag_plugin.cpp impls.cpp leco-driver.cpp "
            "phase1.cpp -o phase1.exe");

  puts("Using Phase 1 to build final stage");
  run("." SEP "phase1.exe");

  puts("Doney-devito");
  return 0;
}

int main(int argc, char **argv) try {
  return try_main(argc, argv);
} catch (int n) {
  fprintf(stderr, "child process failed with code %d\n", n);
}
