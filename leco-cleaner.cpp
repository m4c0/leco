#pragma leco tool
#define GOPT_IMPLEMENTATION
#define MINIRENT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "../minirent/minirent.h"
#include "host_target.hpp"
#include "log.hpp"
#include "sim.hpp"

#include <stdio.h>

static bool log_all{};
static const char *target{HOST_TARGET};

static void usage(const char *argv0) {
  fprintf(stderr, R"(
usage: %s [-a] [-t <target>] [-v]

where:
      -a        remove all known deps recursively
      -t        set target to clean (defaults to host target)
      -v        log all removed files

)",
          argv0);
  throw 0;
}

static void rm_rf(sim_sb *path) {
  DIR *dir = opendir(path->buffer);
  if (dir != nullptr) {
    dirent *ds;
    while ((ds = readdir(dir)) != nullptr) {
      if (ds->d_name[0] == '.')
        continue;
      sim_sb_path_append(path, ds->d_name);
      rm_rf(path);
      sim_sb_path_parent(path);
    }
    closedir(dir);
  }
  if (log_all)
    vlog("removing", path->buffer);
  unlink(path->buffer);
}

static void remove_with_deps(const char *path) {}

int main(int argc, char **argv) try {
  struct gopt opts;
  GOPT(opts, argc, argv, "av");

  bool all{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'a':
      all = true;
      break;
    case 't':
      target = val;
      break;
    case 'v':
      log_all = true;
      break;
    default:
      usage(argv[0]);
      break;
    }
  }
  if (opts.argc != 0)
    usage(argv[0]);

  if (all) {
    remove_with_deps();
  } else {
  }

  return 0;
} catch (...) {
  return 1;
}
