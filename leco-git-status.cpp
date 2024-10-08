#pragma leco tool
#include "sim.hpp"

#include <stdio.h>
#include <string.h>

import mtime;
import popen;
import pprent;
import sys;

#ifdef _WIN32
#define strdup _strdup
#endif

template <unsigned N>
static auto starts_with(const char *str, const char (&prefix)[N]) {
  return 0 == strncmp(str, prefix, N - 1);
}

int main(int argc, char **argv) try {
  for (auto file : pprent::list("..")) {
    if (file[0] == '.') continue;

    if (sim_path_extension(file) != nullptr) continue;

    sim_sbt path{};
    sim_sb_printf(&path, "../%s/.git/config", file);
    if (mtime::of(path.buffer) == 0) continue;

    sim_sbt pwd{};
    sim_sb_printf(&pwd, "../%s", file);

    char *args[7] {};
    args[0] = strdup("git");
    args[1] = strdup("-C");
    args[2] = pwd.buffer;
    args[3] = strdup("status");
    args[4] = strdup("--porcelain=v2");
    args[5] = strdup("--branch");

    p::proc gs{args};

    bool printing{};
    const auto enable_printer = [&] {
      if (printing) return;
      fprintf(stderr, "-=-=-=-=-=-=-=-=-=- %s -=-=-=-=-=-=-=-=-=-\n", file);
      printing = true;
    };

    while (gs.gets()) {
      auto buf = gs.last_line_read();
      if (0 == strcmp(buf, "# branch.ab +0 -0\n")) {
      } else if (starts_with(buf, "# branch.ab")) {
        enable_printer();

        sim_sbt cmd{};
        sim_sb_printf(
            &cmd, "git -C %s log --branches --not --remotes --format=oneline",
            pwd.buffer);
        sys::run(cmd.buffer);
      } else if (buf[0] == '#') {
      } else {
        enable_printer();
        fputs(buf, stderr);
      }
    }
  }
} catch (...) {
  return 1;
}
