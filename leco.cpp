#define MTIME_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../mtime/mtime.h"
#include "die.hpp"
#include "sim.hpp"
#include "targets.hpp"

#include <stdlib.h>

int main(int argc, char **argv) try {
#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  sim_sbt cmd{10240};
  sim_sb_path_copy_real(&cmd, argv[0]);
  sim_sb_path_parent(&cmd);
  sim_sb_path_append(&cmd, "out");
  sim_sb_path_append(&cmd, HOST_TARGET);

  auto argi = 1;
  if (argc < 2) {
    sim_sb_path_append(&cmd, "leco-driver.exe");
  } else {
    sim_sb_path_append(&cmd, "leco-");
    sim_sb_concat(&cmd, argv[1]);
    sim_sb_concat(&cmd, ".exe");
    if (mtime_of(cmd.buffer) > 0) {
      argi++;
    } else {
      sim_sb_path_parent(&cmd);
      sim_sb_path_append(&cmd, "leco-driver.exe");
    }
  }

  for (auto i = argi; i < argc; i++) {
    sim_sb_concat(&cmd, " ");
    sim_sb_concat(&cmd, argv[i]);
  }

  run(cmd.buffer);
} catch (...) {
  return 1;
}
