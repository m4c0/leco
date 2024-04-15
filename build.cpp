#include <stdlib.h>

int main(int argc, char **argv) {
  return system("clang++ -std=c++2b "
                "bouncer.cpp cl.cpp cleaner.cpp compile.cpp context.cpp "
                "dag.cpp dag_io.cpp dag_plugin.cpp evoker.cpp "
                "leco.cpp link.cpp impls.cpp target_defs.cpp "
                "phase1.cpp -o phase1.exe");
}
