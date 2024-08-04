#pragma leco tool
#define SIM_IMPLEMENTATION

#include "dag2.hpp"
#include "sim.hpp"
#include "targets.hpp"

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

static str::set collected{};

void collect_deps(sim_sb *path) {
  if (!collected.insert(path->buffer))
    return;

  sim_sb_path_append(path, "out");
  sim_sb_path_append(path, target);

  for (auto entry : pprent::list(path->buffer)) {
    if (0 != strcmp(".dag", sim_path_extension(entry)))
      continue;

    sim_sb_path_append(path, entry);

    dag_read(path->buffer, [](auto id, auto file) {
      switch (id) {
      case 'impl':
      case 'mdep': {
        sim_sbt p{};
        sim_sb_path_copy_parent(&p, file);
        collect_deps(&p);
        break;
      }
      default:
        break;
      }
    });

    sim_sb_path_parent(path);
  }
}

static void usage() {
  die(R"(
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
    case 'g':
      git = true;
      break;
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
  sim_sb_path_copy_real(&cwd, ".");
  collect_deps(&cwd);

  for (const auto &s : collected) {
    if (git)
      run_git(s.c_str());

    puts(sim_path_filename(s.c_str()));
  }
} catch (...) {
  return 1;
}
