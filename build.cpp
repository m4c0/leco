#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  // TODO: make phase1 leaner
  puts("Building Phase 1");
  if (0 != system("clang++ -std=c++2b "
                  "bouncer.cpp compile.cpp "
                  "dag.cpp dag_plugin.cpp evoker.cpp "
                  "impls.cpp leco.cpp "
                  "phase1.cpp -o phase1.exe"))
    return 1;

  puts("Using Phase 1 to build final stage");
  if (0 != system("./phase1.exe")) {
    return 1;
  }

#if _WIN32
#define OUT "out/x86_64-pc-windows-msvc/leco.exe"
#elif __APPLE__
#define OUT "out/x86_64-apple-macosx11.6.0/leco.exe"
#else
#define OUT "out/x86_64-pc-linux-gnu/leco.exe"
#endif

  puts("Moving final stage to root folder");
  if (0 != rename(OUT, "leco.exe")) {
    perror("failed to rename");
    return 1;
  }

  puts("Doney-devito");
  return 0;
}
