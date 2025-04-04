#pragma leco tool
#include "targets.hpp"

#include <stdlib.h>

import gopt;
import mtime;
import sim;
import sys;
import sysstd;

static void chdir(const char * dir) {
  if (0 != sysstd::chdir(dir)) sys::die("Directory not found: [%s]\n", dir);
}

int main(int argc, char ** argv) try {
#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  auto opts = gopt_parse(argc, argv, "C:", [&](auto ch, auto val) {
    if (ch == 'C') chdir(val);
  });

  if (opts.argc == 0) return sys::tool_run("driver");

  auto cmd = sys::tool_cmd(opts.argv[0]);
  if (mtime::of(*cmd) > 0) {
    opts.argc--;
    opts.argv++;
  } else {
    cmd = sys::tool_cmd("driver");
  }

  // TODO: escape arguments or replace with execvp
  for (auto i = 0; i < opts.argc; i++)
    cmd.printf(" %s", opts.argv[i]);

  sys::run(*cmd);
} catch (...) {
  return 1;
}
