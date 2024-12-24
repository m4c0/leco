#pragma leco tool
#include "targets.hpp"

#include <stdlib.h>

import gopt;
import mtime;
import sim;
import sys;
import sysstd;

static void chdir(const char * dir) {
  if (0 == sysstd::chdir(dir)) return;
  sys::die("Directory not found: [%s]\n", dir);
}

int main(int argc, char ** argv) try {
#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  auto opts = gopt_parse(argc, argv, "C:", [&](auto ch, auto val) {
    switch (ch) {
      case 'C': chdir(val); break;
      default: break;
    }
  });

  if (opts.argc == 0) {
    sys::tool_run("driver");
    return 0;
  }

  auto cmd = sys::tool_cmd(opts.argv[0]);
  if (mtime::of(*cmd) > 0) {
    opts.argc--;
    opts.argv++;
  } else {
    cmd = sys::tool_cmd("driver");
  }

  // TODO: escape arguments
  for (auto i = 0; i < opts.argc; i++)
    cmd.printf(" %s", opts.argv[i]);

  sys::run(*cmd);
} catch (...) {
  return 1;
}
