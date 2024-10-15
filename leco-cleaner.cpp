#pragma leco tool

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
import sim;
import strset;
import sys;

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

static void rm_rf(const char * p) {
  for (auto entry : pprent::list(p)) {
    if (entry[0] == '.') continue;

    auto path = sim::sb { p } / entry;
    rm_rf(*path);
  }

  if (log_all) sys::log("removing", p);
  unlink(p);
#ifndef _WIN32
  rmdir(p);
#endif
}

static str::set temp{};
static void remove_with_deps(const char * p) {
  if (!temp.insert(p)) return;

  auto path = sim::sb { p } / "out" / target;

  for (auto entry : pprent::list(*path)) {
    auto dag = path / entry;

    if (0 != strcmp(".dag", dag.path_extension())) continue;

    sys::dag_read(*dag, [&](auto id, auto file) {
      switch (id) {
        case 'impl':
        case 'mdep': remove_with_deps(*sim::path_parent(file)); break;
        default: break;
      }
    });
  }

  if (!log_all) sys::log("removing", *path);
  rm_rf(*path);
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

  auto cwd = "."_real;
  if (all) {
    remove_with_deps(*cwd);
  } else {
    auto tgt = cwd / "out" / target;
    if (!log_all) sys::log("removing", *tgt);
    rm_rf(*tgt);
  }

  return 0;
} catch (...) {
  return 1;
}
