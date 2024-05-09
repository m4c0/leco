#pragma leco tool
#pragma leco add_impl actool
#pragma leco add_impl bouncer
#pragma leco add_impl bundler
#pragma leco add_impl cl
#pragma leco add_impl cleaner
#pragma leco add_impl dag
#pragma leco add_impl dag_plugin
#pragma leco add_impl droid_path
#pragma leco add_impl evoker
#pragma leco add_impl impls
#pragma leco add_impl plist
#pragma leco add_impl target_defs

#include "bouncer.hpp"
#include "cl.hpp"
#include "dag.hpp"
#include "fopen.hpp"

#include "../pprent/pprent.hpp"

#include <errno.h>
#include <string.h>

static bool error() {
  perror("could not list current directory");
  return false;
}

bool run_target() {
  dag::clear_cache();

  for (auto file : pprent::list(".")) {
    bounce(file);
    errno = 0;
  }
  return errno ? error() : true;
}
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

const char *leco_argv0;
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
