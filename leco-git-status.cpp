#pragma leco tool
#define SIM_IMPLEMENTATION
#include "die.hpp"
#include "sim.hpp"

#include <stdio.h>

import mtime;
import pprent;

int main(int argc, char **argv) try {
  for (auto file : pprent::list("..")) {
    if (file[0] == '.')
      continue;

    if (sim_path_extension(file) != nullptr)
      continue;

    sim_sbt path{};
    sim_sb_printf(&path, "../%s/.git/config", file);
    if (mtime::of(path.buffer) == 0)
      continue;

    fprintf(stderr, "-=-=-=-=-=-=-=-=-=- %s -=-=-=-=-=-=-=-=-=-\n", file);

    sim_sbt cmd{};
    sim_sb_printf(&cmd, "git -C ../%s status", file);
    run(cmd.buffer);
  }
} catch (...) {
  return 1;
}
