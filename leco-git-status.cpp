#pragma leco tool
#define SIM_IMPLEMENTATION
#include "die.hpp"
#include "sim.hpp"

#include <stdio.h>

import pprent;

int main(int argc, char **argv) try {
  for (auto file : pprent::list("..")) {
    if (file[0] == '.')
      continue;

    if (sim_path_extension(file) != nullptr)
      continue;

    printf("-=-=-=-=-=-=-=-=-=- %s -=-=-=-=-=-=-=-=-=-\n", file);

    sim_sbt cmd{};
    sim_sb_printf(&cmd, "git -C ../%s status", file);
    run(cmd.buffer);
  }
} catch (...) {
  return 1;
}
