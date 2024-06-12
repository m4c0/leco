#pragma leco tool
#pragma leco add_impl cl
#pragma leco add_impl target_defs

#define GOPT_IMPLEMENTATION
#define PPRENT_IMPLEMENTATION
#define SIM_IMPLEMENTATION
#include "cl.hpp"
#include "context.hpp"
#include "die.hpp"
#include "fopen.hpp"
#include "gopt.hpp"
#include "sim.hpp"
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

bool run_target() {
  cleaner();

  for (auto file : pprent::list(".")) {
    auto ext = sim_path_extension(file);
    if (ext == nullptr)
      continue;

    if (strcmp(ext, ".cppm") != 0 && strcmp(ext, ".cpp") != 0)
      continue;

    sim_sbt cmd{};
    prep(&cmd, "leco-recurse.exe");
    sim_sb_concat(&cmd, " -t ");
    sim_sb_concat(&cmd, cur_ctx().target.c_str());
    sim_sb_concat(&cmd, " -i ");
    sim_sb_concat(&cmd, file);
    if (is_optimised())
      sim_sb_concat(&cmd, " -g");
    if (is_optimised())
      sim_sb_concat(&cmd, " -O");
    run(cmd.buffer);

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
