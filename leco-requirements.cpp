#pragma leco tool
#include "sim.hpp"
#include "targets.hpp"

#include <stdio.h>
#include <string.h>

import gopt;
import popen;
import pprent;
import strset;
import sys;

#if _WIN32
#define strdup _strdup
#endif

static const char *target{HOST_TARGET};
static const char *argv0;

static str::set all_deps{};
static str::set collected{};

void recurse(const char * dag) {
  if (!all_deps.insert(dag)) return;

  sim_sbt path {};
  sim_sb_path_copy_parent(&path, dag);
  sim_sb_path_parent(&path);
  sim_sb_path_parent(&path);
  collected.insert(path.buffer);

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'bdag':
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
      strdup("git"),       strdup("-C"),   strdup(path),
      strdup("rev-parse"), strdup("HEAD"),
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

  sim_sbt cwd{};
  sim_sb_path_copy_real(&cwd, ".");
  sim_sb_path_append(&cwd, "out");
  sim_sb_path_append(&cwd, target);

  for (auto entry : pprent::list(cwd.buffer)) {
    auto ext = sim_path_extension(entry);
    if (!ext || 0 != strcmp(ext, ".dag")) continue;

    sim_sb_path_append(&cwd, entry);
    recurse(cwd.buffer);
    sim_sb_path_parent(&cwd);
  }

  for (const auto &s : collected) {
    if (git)
      run_git(s.c_str());

    puts(sim_path_filename(s.c_str()));
  }
} catch (...) {
  return 1;
}
