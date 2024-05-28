#pragma leco tool
#pragma leco add_impl actool
#pragma leco add_impl bouncer
#pragma leco add_impl bundler
#pragma leco add_impl cl
#pragma leco add_impl dag
#pragma leco add_impl dag_plugin
#pragma leco add_impl droid_path
#pragma leco add_impl impls
#pragma leco add_impl plist
#pragma leco add_impl target_defs

#include "cl.hpp"
#include "dag.hpp"
#include "die.hpp"
#include "fopen.hpp"
#include "host_target.hpp"

#include "../pprent/pprent.hpp"

#include <errno.h>
#include <string.h>

const char *leco_argv0;

void prep(sim_sb *cmd, const char *tool) {
  sim_sb_path_copy_parent(cmd, leco_argv0);
  sim_sb_path_append(cmd, tool);
}

static bool error() {
  perror("could not list current directory");
  return false;
}

static void cleaner() {
  if (!should_clean_current())
    return;

  sim_sbt cmd{};
  prep(&cmd, "leco-cleaner.exe");
  if (is_extra_verbose()) {
    sim_sb_concat(&cmd, " -v");
  }
  if (should_clean_all()) {
    sim_sb_concat(&cmd, " -a");
  }
  run(cmd.buffer);
}

void bounce(const char *path);
bool run_target() {
  dag::clear_cache();
  cleaner();

  for (auto file : pprent::list(".")) {
    bounce(file);
    errno = 0;
  }
  return errno ? error() : true;
}

extern "C" int main(int argc, char **argv) {
  leco_argv0 = argv[0];
#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  if (!parse_args(argc, argv))
    return 1;

  try {
    if (!for_each_target(run_target))
      return 1;

    return 0;
  } catch (const std::exception &e) {
    fprintf(stderr, "exception: %s\n", e.what());
    return 1;
  } catch (...) {
    fprintf(stderr, "unexpected exception\n");
    return 1;
  }
}
