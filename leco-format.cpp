#pragma leco tool
#define SIM_IMPLEMENTATION
#include "sim.hpp"

import gopt;
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

static bool dry_run{};

static void setup_cmd(sim_sb *cmd) {
  sim_sb_copy(cmd, fmt_cmd());
  sim_sb_printf(cmd, " -i");
  if (dry_run) {
    sim_sb_concat(cmd, " -n --Werror");
  }
}

static void work_from_git() {
  char *args[]{
      strdup("git"),
      strdup("status"),
      strdup("--porcelain=v2"),
      0,
  };

  sim_sbt cmd{10240};
  setup_cmd(&cmd);

  unsigned count{};
  p::proc p{args};
  while (p.gets()) {
    auto line = p.last_line_read();
    if (strncmp(line, "1 D. ", 5) == 0) {
      count++;
      continue;
    }
    if (strstr(line, ".cpp") == nullptr && strstr(line, ".mm") == nullptr)
      continue;

    auto file = strrchr(line, ' ') + 1;
    auto sep = strchr(file, '\t');
    int len = sep ? (sep - file) : strlen(file) - 1;

    sim_sb_printf(&cmd, " %.*s", len, file);
    count++;
  }

  if (count == 0) {
    if (dry_run)
      return;

    sys::die("missing input files");
  }

  sys::run(cmd.buffer);
}

int main(int argc, char **argv) try {
  auto opts = gopt_parse(argc, argv, "n", [](auto ch, auto val) {
    switch (ch) {

    case 'n':
      dry_run = true;
      break;
    }
  });

  if (opts.argc == 0) {
    work_from_git();
    return 0;
  }

  sim_sbt cmd{10240};
  setup_cmd(&cmd);

  for (auto i = 0; i < opts.argc; i++) {
    if (mtime::of(opts.argv[i]) == 0) {
      sys::die("file not found: %s", opts.argv[i]);
    }

    sim_sb_printf(&cmd, " %s", opts.argv[i]);
  }

  sys::run(cmd.buffer);
} catch (...) {
  return 1;
}
