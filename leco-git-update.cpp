#pragma leco tool

#include "sim.hpp"
#include "targets.hpp"

#include <string.h>

import gopt;
import pprent;
import strset;
import sys;

static const char *target{HOST_TARGET};
static const char *argv0;

static str::set unique_parents{};

static str::set added{};
static void read_dag(const char *dag) {
  if (!added.insert(dag))
    return;

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
    case 'idag':
    case 'mdag': {
      read_dag(file);
      break;
    }
    default:
      break;
    }
  });

  sim_sbt parent{};
  sim_sb_path_copy_parent(&parent, dag);
  sim_sb_path_parent(&parent);
  sim_sb_path_parent(&parent);
  unique_parents.insert(parent.buffer);
}

static void usage() {
  sys::die(R"(
Usage: %s -t <target>

Where:
        -t: Target triple to scan. The list might vary based on target
            depending on the platform-specifics of each dependency.
)",
      argv0);
}

int main(int argc, char **argv) try {
  argv0 = argv[0];
  auto opts = gopt_parse(argc, argv, "t:", [&](auto ch, auto val) {
    switch (ch) {
    case 't':
      target = val;
      break;
    default:
      usage();
    }
  });
  if (opts.argc != 0)
    usage();

  sim_sbt cwd{};
  sim_sb_path_copy_real(&cwd, "out");
  sim_sb_path_append(&cwd, target);
  for (auto entry : pprent::list(cwd.buffer)) {
    auto ext = sim_path_extension(entry);
    if (!ext || (0 != strcmp(".dag", ext)))
      continue;

    sim_sbt file{};
    sim_sb_path_copy_append(&file, cwd.buffer, entry);
    read_dag(file.buffer);
  }

  for (auto &parent : unique_parents) {
    sim_sbt cmd{};
    sim_sb_copy(&cmd, "git -C ../");
    sim_sb_concat(&cmd, sim_path_filename(parent.c_str()));
    sim_sb_concat(&cmd, " pull");
    sys::run(cmd.buffer);
  }
} catch (...) {
  return 1;
}
