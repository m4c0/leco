#define SIM_IMPLEMENTATION

#include "die.hpp"
#include "sim.h"
#include "targets.hpp"

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include <stdlib.h>

import gopt;
import mtime;

int main(int argc, char **argv) try {
#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  sim_sb cmd{};
  sim_sb_new(&cmd, 10240);
  sim_sb_path_copy_real(&cmd, argv[0]);
  sim_sb_path_parent(&cmd);
  sim_sb_path_append(&cmd, "out");
  sim_sb_path_append(&cmd, HOST_TARGET);

  auto opts = gopt_parse(argc, argv, "C:", [&](auto ch, auto val) {
    switch (ch) {
    case 'C':
      if (0 != chdir(val)) {
        die("Directory not found: [%s]\n", val);
      }
      break;
    default:
      printf("[%c][%s]\n", ch, val);
      break;
    }
  });

  if (opts.argc == 0) {
    sim_sb_path_append(&cmd, "leco-driver.exe");
    run(cmd.buffer);
    return 0;
  }

  sim_sb_path_append(&cmd, "leco-");
  sim_sb_concat(&cmd, opts.argv[0]);
  sim_sb_concat(&cmd, ".exe");

  if (mtime::of(cmd.buffer) > 0) {
    opts.argc--;
    opts.argv++;
  } else {
    sim_sb_path_parent(&cmd);
    sim_sb_path_append(&cmd, "leco-driver.exe");
  }

  // TODO: escape arguments
  for (auto i = 0; i < opts.argc; i++) {
    sim_sb_concat(&cmd, " ");
    sim_sb_concat(&cmd, opts.argv[i]);
  }

  run(cmd.buffer);
} catch (...) {
  return 1;
}
