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

#include "bouncer.hpp"
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

bool run_target() {
  dag::clear_cache();
  cleaner();

  for (auto file : pprent::list(".")) {
    bounce(file);
    errno = 0;
  }
  return errno ? error() : true;
}
// TODO: move to another tool
static void dump_deps() {
  std::set<std::string> all_parents{};

  dag::visit_all([&](auto *n) {
    // TODO: maybe consider headers, too
    sim_sbt parent{};
    sim_sb_path_copy_parent(&parent, n->source());
    all_parents.insert(sim_sb_path_filename(&parent));
  });

  FILE *f{};
  if (0 != fopen_s(&f, "out/requirements.txt", "w"))
    return;
  for (const auto &s : all_parents) {
    fprintf(f, "%s\n", s.c_str());
  }
  fclose(f);
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
