#pragma leco tool
#include "sim.h"

#include <stdint.h>
#include <string.h>

import gopt;
import mtime;
import pprent;
import sim;
import sys;

static const char *target;

static void bounce(const char * src);
static void build_bdeps(const char * dag) {
  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'bdag': bounce(file); break;
      default: break;
    }
  });
}

static void bounce(const char * dag) {
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
    case 'tapp':
      build_bdeps(dag);
      // build_rc(src);
      break;
    default: break;
    }
  });
}

static void usage() { sys::die("invalid usage"); }

int main(int argc, char **argv) try {
  sim::sb flags{};
  const char * input {};

  auto opts = gopt_parse(argc, argv, "t:i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i': input = val; break;
    case 't': target = val; break;
    default: usage(); break;
    }
  });
  if (opts.argc != 0) usage();
  if (!input && !target) usage();

  if (input) {
    bounce(input);
    return 0;
  }

  auto path = ("out"_real /= target);
  for (auto file : pprent::list(*path)) {
    auto ext = sim_path_extension(file);
    if (!ext || 0 != strcmp(ext, ".dag")) continue;

    path /= file;
    bounce(*path);
    sim_sb_path_parent(&path);
  }
} catch (...) {
  return 1;
}
