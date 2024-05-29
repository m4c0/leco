#define SIM_IMPLEMENTATION

#include "die.hpp"
#include "sim.hpp"
#include "targets.hpp"

#include <stdlib.h>

int main(int argc, char **argv) try {
  sim_sbt cmd{10240};
  sim_sb_path_copy_real(&cmd, argv[0]);
  sim_sb_path_parent(&cmd);
  sim_sb_path_append(&cmd, "out");
  sim_sb_path_append(&cmd, HOST_TARGET);
  sim_sb_path_append(&cmd, "leco-driver.exe");

  for (auto i = 1; i < argc; i++) {
    sim_sb_concat(&cmd, " ");
    sim_sb_concat(&cmd, argv[i]);
  }

  run(cmd.buffer);
} catch (...) {
  return 1;
}
