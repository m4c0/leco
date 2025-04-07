#include "sim.h"
#include "targets.hpp"

#define SEP SIM_PATHSEP_S
#define CMD ".." SEP "leco" SEP "out" SEP HOST_TARGET SEP "leco-meta.exe"

// Using Windows definition for extra constness
extern "C" int execv(const char *, const char * const *);

int main(int argc, const char ** argv) {
  if (argv[argc]) return 69;
  argv[0] = CMD;
  return execv(argv[0], argv);
}
