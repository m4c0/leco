#include <stdlib.h>

int main(int argc, char **argv) {
  if (0 != system("clang++ -std=c++2b "
                  "bouncer.cpp cl.cpp cleaner.cpp compile.cpp context.cpp "
                  "dag.cpp dag_plugin.cpp evoker.cpp "
                  "leco.cpp link.cpp impls.cpp target_defs.cpp "
                  "phase1.cpp -o phase1.exe"))
    return 1;

  // TODO: make phase1 copy the final leco as its sibling
  return system("./phase1.exe");
}
