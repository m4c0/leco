#pragma leco tool

#include "sim.hpp"
#include "targets.hpp"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define unlink _unlink
#else
#include <unistd.h>
#endif

import gopt;
import pprent;
import sys;
import strset;

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
  for (auto entry : pprent::list(path->buffer)) {
    if (entry[0] == '.')
      continue;
    sim_sb_path_append(path, entry);
    rm_rf(path);
    sim_sb_path_parent(path);
  }
  if (log_all)
    sys::log("removing", path->buffer);

  unlink(path->buffer);
#ifndef _WIN32
  rmdir(path->buffer);
#endif
}

static str::set temp{};
static void remove_with_deps(sim_sb *path) {
  if (!temp.insert(path->buffer))
    return;

  sim_sb_path_append(path, "out");
  sim_sb_path_append(path, target);

  for (auto entry : pprent::list(path->buffer)) {
    if (0 != strcmp(".dag", sim_path_extension(entry)))
      continue;

    sim_sb_path_append(path, entry);

    sys::dag_read(path->buffer, [](auto id, auto file) {
      switch (id) {
      case 'impl':
      case 'mdep': {
        sim_sbt p{};
        sim_sb_path_copy_parent(&p, file);
        remove_with_deps(&p);
        break;
      }
      default:
        break;
      }
    });

    sim_sb_path_parent(path);
  }

  if (!log_all)
    sys::log("removing", path->buffer);

  rm_rf(path);
}

int main(int argc, char **argv) try {
  bool all{};
  auto opts = gopt_parse(argc, argv, "avt:", [&](auto ch, auto val) {
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
  });
  if (opts.argc != 0)
    usage(argv[0]);

  sim_sbt cwd{};
  sim_sb_path_copy_real(&cwd, ".");
  if (all) {
    remove_with_deps(&cwd);
  } else {
    sim_sb_path_append(&cwd, "out");
    sim_sb_path_append(&cwd, target);
    if (!log_all)
      sys::log("removing", cwd.buffer);
    rm_rf(&cwd);
  }

  return 0;
} catch (...) {
  return 1;
}
