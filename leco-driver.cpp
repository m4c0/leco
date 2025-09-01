#pragma leco tool
#include "../mct/mct-syscall.h"
#include "targets.hpp"

#include <string.h>

import sys;

unsigned clean_level{};

static void usage() {
  sys::die(R"(
  Usage: ../leco/leco.exe [-C <dir>] [-g] [-O] [-t <target>]

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
}

static void run_target(const char * target) {
  mct_syscall_setenv("LECO_TARGET", target);

  if (clean_level == 1) sys::tool_run("clean");
  if (clean_level >= 2) sys::tool_run("clean", "-a");

  if (!sys::is_tgt_host()) sys::tool_run("sysroot");

  sys::    tool_run("dagger");
  sys::opt_tool_run("shaders");
  sys::opt_tool_run("rc");
  sys::opt_tool_run("deplist");
  sys::    tool_run("pcm");
  sys::    tool_run("obj");
  sys::    tool_run("pcm2obj");
  sys::    tool_run("link");
  sys::opt_tool_run("test");
  sys::opt_tool_run("exs");
  sys::opt_tool_run("rsrc");

  if (sys::is_tgt_apple()) sys::tool_run("ipa");
  if (sys::is_tgt_wasm())  sys::tool_run("wasm-js");
}

static const char * target_of(const char * tgt) {
  sim::sb target { tgt };

#ifdef __APPLE__
  if (target == "host" || target == "macosx") return TGT_OSX;
  if (target == "iphoneos") return TGT_IPHONEOS;
  if (target == "iphonesimulator") return TGT_IOS_SIMULATOR;
  if (target == "android") return TGT_DROID_ARMV7;
#elifdef _WIN32
  if (target == "host" || target == "windows") return TGT_WINDOWS;
  if (target == "android") return TGT_DROID_X86_64;
#elifdef __linux__
  if (target == "host" || target == "linux") return TGT_LINUX;
  if (target == "android") return TGT_DROID_X86_64;
#endif

  if (target == "wasm") return TGT_WASM;

  if (target == "android_aarch64") return TGT_DROID_AARCH64;
  if (target == "android_armv7")   return TGT_DROID_ARMV7;
  if (target == "android_x86")     return TGT_DROID_X86;
  if (target == "android_x86_64")  return TGT_DROID_X86_64;

  sys::die("unknown or invalid target for this platform: %s", *target);
}

static void chdir(const char * dir) {
  if (0 != mct_syscall_chdir(dir)) sys::die("Directory not found: [%s]\n", dir);
}

int main(int argc, char ** argv) try {
  const auto shift = [&] { return argc > 1 ? (argc--, *++argv) : nullptr; };
  const char * target = target_of("host");
  while (auto val = shift()) {
    if ("-c"_s == val) clean_level++;
    else if ("-C"_s == val) chdir(shift());
    else if ("-t"_s == val) target = target_of(shift());
    else if ("-g"_s == val) mct_syscall_setenv("LECO_DEBUG", "1");
    else if ("-O"_s == val) mct_syscall_setenv("LECO_OPT", "1");
    else usage();
  }

  run_target(target);
  return 0;
} catch (...) {
  return 1;
}
