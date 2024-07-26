#pragma leco tool
#define SIM_IMPLEMENTATION
#include "sim.hpp"

import mtime;
import popen;
import sys;

#ifdef _WIN32
#define strdup _strdup
#endif

static const char *fmt_cmd() {
#if __APPLE__
  return "/usr/local/opt/llvm@16/bin/clang-format";
#elif _WIN32
  return "clang-format.exe";
#else
  return "clang-format";
#endif
}

static void work_from_git() {
  char *args[5]{};
  args[0] = strdup("git");
  args[1] = strdup("status");
  args[2] = strdup("--porcelain=v2");
  args[3] = strdup("-z");

  sim_sbt cmd{10240};
  sim_sb_copy(&cmd, fmt_cmd());
  sim_sb_concat(&cmd, " -i");

  unsigned count{};
  p::proc p{args};
  while (p.gets()) {
    auto line = p.last_line_read();
    auto file = strrchr(line, ' ') + 1;
    sim_sb_printf(&cmd, " %s", file);
    count++;
  }

  if (count == 0) {
    sys::die("missing input files");
  }

  sys::run(cmd.buffer);
}

int main(int argc, char **argv) try {
  if (argc == 1) {
    work_from_git();
    return 0;
  }

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
