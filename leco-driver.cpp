#pragma leco tool
#include "sim.hpp"
#include "targets.hpp"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

import gopt;
import pprent;
import sys;

const char *leco_argv0;
const char *common_flags;
unsigned clean_level{};

static void usage() {
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

static void prep(sim_sb *cmd, const char *tool) {
  sim_sb_path_copy_parent(cmd, leco_argv0);
  sim_sb_path_append(cmd, tool);
}

static bool error() {
  perror("could not list current directory");
  throw 0;
}

static void cleaner(const char *target) {
  if (clean_level == 0)
    return;

  sim_sbt cmd{};
  prep(&cmd, "leco-cleaner.exe");
  sim_sb_printf(&cmd, " -t %s", target);
  if (clean_level > 1) {
    sim_sb_concat(&cmd, " -a");
  }
  sys::run(cmd.buffer);
}

static void sysroot(const char *target) {
  if (0 == strcmp(target, HOST_TARGET))
    return;

  sim_sbt cmd{};
  prep(&cmd, "leco-sysroot.exe");
  sim_sb_printf(&cmd, " -q -t %s", target);
  sys::run(cmd.buffer);
}

static void recurse(const char * target, const char *file) {
  sim_sbt cmd{};
  prep(&cmd, "leco-recurse.exe");
  sim_sb_printf(&cmd, " -t %s", target);
  sim_sb_printf(&cmd, " -i %s ", file);
  sim_sb_concat(&cmd, common_flags);
  sys::run(cmd.buffer);
}

static void run_target(const char *target) {
  cleaner(target);
  sysroot(target);

  for (auto file : pprent::list(".")) {
    auto ext = sim_path_extension(file);
    if (ext == nullptr)
      continue;

    if (strcmp(ext, ".cppm") != 0 && strcmp(ext, ".cpp") != 0 &&
        strcmp(ext, ".c") != 0)
      continue;

    recurse(target, file);

    errno = 0;
  }
  if (errno)
    error();
}

static void run_targets(const char *target) {
#ifdef __APPLE__
  if (0 == strcmp(target, "apple")) {
    run_target(TGT_OSX);
    run_target(TGT_IPHONEOS);
    run_target(TGT_IOS_SIMULATOR);
    return;
  }

  if (0 == strcmp(target, "ios")) {
    run_target(TGT_IPHONEOS);
    run_target(TGT_IOS_SIMULATOR);
    return;
  }

  if (0 == strcmp(target, "host") || 0 == strcmp(target, "macosx")) {
    run_target(TGT_OSX);
    return;
  }
  if (0 == strcmp(target, "iphoneos")) {
    run_target(TGT_IPHONEOS);
    return;
  }
  if (0 == strcmp(target, "iphonesimulator")) {
    run_target(TGT_IOS_SIMULATOR);
    return;
  }
#endif

#ifdef _WIN32
  if (0 == strcmp(target, "host") || 0 == strcmp(target, "windows")) {
    run_target(TGT_WINDOWS);
    return;
  }
#endif

#ifdef __linux__
  if (0 == strcmp(target, "host") || 0 == strcmp(target, "linux")) {
    run_target(TGT_LINUX);
    return;
  }
#endif

  if (0 == strcmp(target, "android")) {
    run_target(TGT_DROID_AARCH64);
    run_target(TGT_DROID_ARMV7);
    run_target(TGT_DROID_X86);
    run_target(TGT_DROID_X86_64);
    return;
  }
  if (0 == strcmp(target, "wasm")) {
    run_target(TGT_WASM);
    return;
  }

  if (IS_TGT_DROID(target) || IS_TGT_APPLE(target) || IS_TGT(target, TGT_WINDOWS) || IS_TGT(target, TGT_LINUX)
      || IS_TGT(target, TGT_WASM)) {
    run_target(target);
    return;
  }

  sys::die("unknown target: %s", target);
}

extern "C" int main(int argc, char **argv) try {
  leco_argv0 = argv[0];

  const char *target{"host"};
  sim_sbt flags{};
  auto opts = gopt_parse(argc, argv, "cgOt:", [&](auto ch, auto val) {
    switch (ch) {
    case 'c': clean_level++; break;
    case 'g': sim_sb_concat(&flags, " -g"); break;
    case 'O': sim_sb_concat(&flags, " -O"); break;
    case 't': target = val; break;
    default: usage(); break;
    }
  });
  if (opts.argc != 0) usage();

  common_flags = flags.buffer;
  run_targets(target);
  return 0;
} catch (...) {
  return 1;
}
