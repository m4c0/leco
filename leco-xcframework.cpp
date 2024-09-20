#pragma leco app

#include "sim.hpp"

import gopt;
import strset;
import sys;

static const char * target;

static void usage() {
  sys::die(R"(invalid usage)");
}

static void xcfw(const char * fw) {
  // sys::log(target, fw);
}

static str::set processed {};
static void run(const char * dag) {
  if (!processed.insert(dag)) return;

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'xcfw': xcfw(file); break;
      case 'bdag':
      case 'idag':
      case 'mdag': run(file); break;
      default: break;
    }
  });
}

int main(int argc, char ** argv) try {
  const char * input {};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      default: usage(); break;
    }
  });
  if (!input || opts.argc) usage();

  sim_sbt path {};
  sim_sb_copy(&path, input);
  sim_sb_path_parent(&path);
  target = sim_sb_path_filename(&path);

  run(input);
} catch (...) {
  return 1;
}
