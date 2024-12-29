#pragma leco tool

#include "targets.hpp"

#include <string.h>

import gopt;
import pprent;
import sim;
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

  auto parent = sim::path_parent(dag);
  parent.path_parent();
  parent.path_parent();
  unique_parents.insert(*parent);
}

static void usage() {
  sys::die(R"(
Runs a "git pull" for each of the collected dependencies of the given target.

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

  auto cwd = "out"_real / target;
  for (auto entry : pprent::list(cwd.buffer)) {
    auto ext = sim::path_extension(entry);
    if (!ext.len || (0 != strcmp(".dag", *ext)))
      continue;

    read_dag(*(cwd / entry));
  }

  for (auto & parent : unique_parents) {
    sys::runf("git -C %s pull", parent.c_str());
  }
} catch (...) {
  return 1;
}
