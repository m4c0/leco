#pragma leco tool
#define SIM_IMPLEMENTATION
#include "sim.hpp"

import gopt;
import sys;

static void usage() { sys::die("invalid usage"); }

int main(int argc, char **argv) {
  const char *input;
  sim_sbt appdir{};

  auto opts = gopt_parse(argc, argv, "i:a:", [&](auto ch, auto val) {
    switch (ch) {
    case 'a':
      sim_sb_path_copy_real(&appdir, val);
      break;
    case 'i':
      input = val;
      break;
    default:
      usage();
    }
  });

  if (opts.argc != 0 || !input || !appdir.len)
    usage();

  sim_sb_path_append(&appdir, "leco.js");
  sys::log("copying", appdir.buffer);
  sys::link("../leco/wasm.js", appdir.buffer);
}
