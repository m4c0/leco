#pragma leco tool
#define SIM_IMPLEMENTATION
#include "sim.hpp"

import sys;

int main(int argc, char ** argv) {
  sim_sbt cmd { 10240 };
  sim_sb_path_copy_real(&cmd, ".");
  sim_sb_path_parent(&cmd);
  sim_sb_path_append(&cmd, "leco");
  sim_sb_path_append(&cmd, "leco.exe");

  for (auto i = 1; i < argc; i++) {
    sim_sb_printf(&cmd, " %s", argv[i]);
  }

  sys::run(cmd.buffer);
  sys::run("python3 -mhttp.server");
}
