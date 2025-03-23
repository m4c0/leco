#pragma leco tool
#include "targets.hpp"

#include <stdio.h>
#include <string.h>

import gopt;
import sim;
import sys;

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

static void cleaner(const char *target) {
  if (clean_level == 0) return;

  const char * lvl = (clean_level > 1) ? "-a" : "";
  sys::tool_run("cleaner", "-t %s %s", target, lvl);
}

static void sysroot(const char *target) {
  if (0 == strcmp(target, HOST_TARGET)) return;
  sys::tool_run("sysroot", "-q -t %s", target);
}

static void dagger(const char * target) {
  sys::tool_run("dagger", "-t %s", target);
}

static void embed(const char * target) {
  sys::tool_run("embed", "-t %s", target);
}

static void bundle(const char * target) {
  sys::tool_run("bundler", "-t %s", target);
}

static void shaders(const char * target) {
  sys::for_each_dag(target, false, [](auto * dag, auto id, auto file) {
    if (id != 'tapp') return;
    sys::tool_run("shaders", "-i %s", dag);
  });
}

static void compile(const char * target) {
  sys::for_each_dag(target, false, [](auto * dag, auto id, auto file) {
    switch (id) {
      case 'tapp':
#if _WIN32
        sys::tool_run("rc", "-i %s", dag);
#endif
      case 'tdll':
      case 'tool':
      case 'tmmd': 
        sys::tool_run("pcm", "-i %s %s", dag, common_flags);
        sys::tool_run("obj", "-i %s %s", dag, common_flags);
        break;
      default: break;
    }
  });
}

static void link(const char * target) {
  sys::for_each_dag(target, false, [](auto * dag, auto id, auto file) {
    switch (id) {
      case 'tapp':
      case 'tdll':
      case 'tool':
        sys::tool_run("link", "-i %s -o %s %s", dag, file, common_flags);
        break;
      default: break;
    }
  });
}

static void run_target(const char * target) {
  cleaner(target);
  sysroot(target);
  dagger(target);
  shaders(target);
  embed(target);
  compile(target);
  link(target);
  bundle(target);
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
  const char *target{"host"};
  sim::sb flags {};
  auto opts = gopt_parse(argc, argv, "cgOt:", [&](auto ch, auto val) {
    switch (ch) {
    case 'c': clean_level++; break;
    case 'g': flags += " -g"; break;
    case 'O': flags += " -O"; break;
    case 't': target = val; break;
    default: usage(); break;
    }
  });
  if (opts.argc != 0) usage();

  common_flags = *flags;
  run_targets(target);
  return 0;
} catch (...) {
  return 1;
}
