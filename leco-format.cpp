#pragma leco tool
#define SIM_IMPLEMENTATION
#include "sim.hpp"

import mtime;
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

int main(int argc, char **argv) try {
  sim_sbt cmd{10240};
  sim_sb_copy(&cmd, fmt_cmd());
  sim_sb_concat(&cmd, " -i");

  for (auto i = 1; i < argc; i++) {
    if (mtime::of(argv[i]) == 0) {
      sys::die("file not found: %s", argv[i]);
    }

    sim_sb_printf(&cmd, " %s", argv[i]);
  }

  sys::run(cmd.buffer);
} catch (...) {
  return 1;
}
