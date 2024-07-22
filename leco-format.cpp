#pragma leco tool
#define SIM_IMPLEMENTATION
#include "sim.hpp"

import gopt;
import sys;

static const char *fmt_cmd() {
#if __APPLE__
  return "/usr/local/opt/llvm@16/bin/clang-format";
#elif _WIN32
  return "clang-format.exe";
#else
  return "clang-format";
#endif
}

static void usage() { sys::die("invalid usage"); }

int main(int argc, char **argv) try {
  const char *input{};

  // TODO: add an embedded config to be used if ".clang-format" does not exist
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    default:
      usage();
    }
  });

  // TODO: default to format all files in current dir
  if (!input || opts.argc != 0)
    usage();

  sim_sbt cmd{10240};
  sim_sb_printf(&cmd, "%s -i %s", fmt_cmd(), input);
  sys::run(cmd.buffer);
} catch (...) {
  return 1;
}
