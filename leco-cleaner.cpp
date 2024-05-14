#pragma leco tool
#define GOPT_IMPLEMENTATION
#define PPRENT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "../pprent/pprent.hpp"
#include "die.hpp"
#include "fopen.hpp"
#include "host_target.hpp"
#include "log.hpp"
#include "sim.hpp"

#include <stdio.h>

#ifndef _WIN32
#include <unistd.h>
#endif

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
    log("removing", path->buffer);

  unlink(path->buffer);
#ifndef _WIN32
  rmdir(path->buffer);
#endif
}

#include <set>
#include <string>
static std::set<std::string> temp{};
static void remove_with_deps(sim_sb *path) {
  auto [_, x] = temp.emplace(path->buffer);
  if (!x)
    return;

  sim_sb_path_append(path, "out");
  sim_sb_path_append(path, target);

  for (auto entry : pprent::list(path->buffer)) {
    if (0 != strcmp(".dag", sim_path_extension(entry)))
      continue;

    sim_sb_path_append(path, entry);
    FILE *f{};
    if (0 != fopen_s(&f, path->buffer, "r"))
      die("failed to remove: %s", path->buffer);

    char buf[10240];
    while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
      if (strlen(buf) < 5)
        die("invalid line in dag file [%s]", path->buffer);

      uint32_t *id = reinterpret_cast<uint32_t *>(buf);
      char *file = reinterpret_cast<char *>(id + 1);
      file[strlen(file) - 1] = 0;

      switch (*id) {
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
    }

    fclose(f);
    sim_sb_path_parent(path);
  }

  if (!log_all)
    log("removing", path->buffer);

  rm_rf(path);
}

int main(int argc, char **argv) try {
  struct gopt opts;
  GOPT(opts, argc, argv, "avt:");

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

  sim_sbt cwd{};
  sim_sb_path_copy_real(&cwd, ".");
  if (all) {
    remove_with_deps(&cwd);
  } else {
    sim_sb_path_append(&cwd, "out");
    sim_sb_path_append(&cwd, target);
    if (!log_all)
      log("removing", cwd.buffer);
    rm_rf(&cwd);
  }

  return 0;
} catch (...) {
  return 1;
}