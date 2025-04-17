#define SYSSTD_IMPLEMENTATION
#include "../sysstd/sysstd.h"
#include "sim.h"
#include "targets.hpp"

#define SEP SIM_PATHSEP_S
#define CMD ".." SEP "leco" SEP "out" SEP HOST_TARGET SEP "leco-meta.exe"

int main(int argc, const char ** argv) {
  if (argv[argc]) return 69;
  argv[0] = CMD;
  return sysstd_spawn(argv[0], argv);
}
