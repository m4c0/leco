#pragma leco tool
#include "targets.hpp"

#include <stdio.h>
#include <string.h>

import gopt;
import popen;
import pprent;
import sim;
import strset;
import sys;
import sysstd;

static const char *target{HOST_TARGET};
static const char *argv0;

static str::set all_deps{};
static str::set collected{};

void recurse(const char * dag) {
  if (!all_deps.insert(dag)) return;

  auto path = sim::path_parent(dag);
  path.path_parent();
  path.path_parent();
  collected.insert(*path);

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'idag':
      case 'mdag': recurse(file); break;
      default: break;
    }
  });
}

static void usage() {
  sys::die(R"(
Usage: %s -t <target> [-g]

Where:
        -t: Target triple to scan. The list might vary based on target
            depending on the platform-specifics of each dependency.
        -g: Print the git commit hash alongside each dep
)",
      argv0);
}

static void run_git(const char *path) {
  char *cmd[6]{
    sysstd::strdup("git"),
    sysstd::strdup("-C"),
    sysstd::strdup(path),
    sysstd::strdup("rev-parse"),
    sysstd::strdup("HEAD"),
  };
  p::proc p{cmd};
  if (!p.gets())
    sys::die("failed to get git status");

  const char *line = p.last_line_read();
  int len = strlen(line);
  printf("%.*s ", len - 1, line);
}

int main(int argc, char **argv) try {
  bool git{};

  argv0 = argv[0];
  auto opts = gopt_parse(argc, argv, "gt:", [&](auto ch, auto val) {
    switch (ch) {
    case 'g': git = true; break;
    case 't': target = val; break;
    default: usage(); break;
    }
  });
  if (opts.argc != 0) usage();

  auto cwd = "."_real / "out" / target;

  collected.insert("../leco");
  for (auto entry : pprent::list(*cwd)) {
    auto ext = sim::path_extension(entry);
    if (ext != ".dag") continue;

    auto e = cwd / entry;
    recurse(*e);
  }

  for (const auto &s : collected) {
    if (git) run_git(s.c_str());
    puts(sim::path_filename(s.c_str()));
  }
} catch (...) {
  return 1;
}
