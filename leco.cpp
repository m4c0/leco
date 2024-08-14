#define SIM_IMPLEMENTATION

#include "sim.h"
#include "targets.hpp"

#define SEP SIM_PATHSEP_S
#define CMD ".." SEP "leco" SEP "out" SEP HOST_TARGET SEP "leco-meta.exe"

int main(int argc, char ** argv) {
  sim_sb cmd {};
  sim_sb_new(&cmd, 10240);
  sim_sb_copy(&cmd, CMD);
  for (auto i = 1; i < argc; i++) {
    // TODO: escape
    sim_sb_printf(&cmd, " %s", argv[i]);
  }
  return system(cmd.buffer);
}
