#pragma leco tool
#include "sim.h"
#include "targets.hpp"

#include <stdlib.h>

import gopt;
import mtime;
import sys;
import sysstd;

int main(int argc, char ** argv) try {
#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  sim_sb cmd {};
  sim_sb_new(&cmd, 10240);
  sim_sb_path_copy_real(&cmd, argv[0]);
  sim_sb_path_parent(&cmd);

  auto opts = gopt_parse(argc, argv, "C:", [&](auto ch, auto val) {
    switch (ch) {
      case 'C':
        if (0 != sysstd::chdir(val)) {
          sys::die("Directory not found: [%s]\n", val);
        }
        break;
      default: break;
    }
  });

  if (opts.argc == 0) {
    sim_sb_path_append(&cmd, "leco-driver.exe");
    sys::run(cmd.buffer);
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

  sys::run(cmd.buffer);
} catch (...) {
  return 1;
}
