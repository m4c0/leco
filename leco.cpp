#include "bouncer.hpp"
#include "cl.hpp"
#include "dag.hpp"

#include "../minirent/minirent.h"

#include <string.h>

static bool error() {
  fprintf(stderr, "Could not list current directory: %s\n", strerror(errno));
  return false;
}

bool run_target() {
  dag::clear_cache();

  DIR *dir = opendir(".");
  if (dir == nullptr)
    return error();

  struct dirent *dp{};
  while ((dp = readdir(dir))) {
    if (!bounce(dp->d_name))
      return false;

    errno = 0;
  }
  if (errno)
    return error();

  closedir(dir);
  return true;
}
static void dump_deps() {
  std::set<std::string> all_parents{};

  dag::visit_all([&](auto *n) {
    sim_sbt parent{};
    sim_sb_path_copy_parent(&parent, n->source());
    all_parents.insert(sim_sb_path_filename(&parent));
  });

  FILE *f = fopen("out/requirements.txt", "w");
  if (!f)
    return;
  for (const auto &s : all_parents) {
    fprintf(f, "%s\n", s.c_str());
  }
  fclose(f);
}

extern "C" int main(int argc, char **argv) {
#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  if (!parse_args(argc, argv))
    return 1;

  try {
    if (!for_each_target(run_target))
      return 1;

    dump_deps();
    return 0;
  } catch (const std::exception &e) {
    fprintf(stderr, "exception: %s\n", e.what());
    return 1;
  } catch (...) {
    fprintf(stderr, "unexpected exception\n");
    return 1;
  }
}
