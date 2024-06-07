#pragma leco tool
#pragma leco add_impl bouncer
#pragma leco add_impl cl
#pragma leco add_impl dag
#pragma leco add_impl dag_plugin
#pragma leco add_impl impls
#pragma leco add_impl target_defs

#include "cl.hpp"
#include "dag.hpp"
#include "die.hpp"
#include "fopen.hpp"
#include "targets.hpp"

#include "../pprent/pprent.hpp"

#include <errno.h>
#include <string.h>

const char *leco_argv0;

void usage() {
  fprintf(stderr, R"(
  Usage: ../leco/leco.exe [-C <dir>] [-D] [-g] [-O] [-t <target>] [-v]

  Where:
    -c -- clean current module before build (if repeated, clean all modules)

    -C -- change to this directory before build

    -g -- enable debug symbols

    -O -- enable optimisations

    -t <target> -- one of:
      iphoneos, iphonesimulator: for its referring platform (requires Apple SDKs)
      ios: for both iPhoneOS and iPhoneSimulator
      android: for all four Android architectures (requires Android SDK)
      apple, linux, macosx, windows: for their respective platforms (requires their SDKs)
      host: for the same platform as the host (default)
)");
  throw 0;
}

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

extern "C" int main(int argc, char **argv) try {
  leco_argv0 = argv[0];
#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  if (!parse_args(argc, argv))
    usage();

  if (!for_each_target(run_target))
    return 1;

  return 0;
} catch (const std::exception &e) {
  fprintf(stderr, "exception: %s\n", e.what());
  return 1;
} catch (...) {
  return 1;
}
