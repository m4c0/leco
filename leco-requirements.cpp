#pragma leco tool
#define SIM_IMPLEMENTATION

#include "dag2.hpp"
#include "die.hpp"
#include "sim.hpp"
#include "targets.hpp"

import gopt;
import pprent;
import strset;

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
  sim_sb_path_copy_real(&cwd, ".");
  collect_deps(&cwd);

  for (const auto &s : collected) {
    puts(sim_path_filename(s.c_str()));
  }
} catch (...) {
  return 1;
}
