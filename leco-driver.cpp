#pragma leco tool
#include "../mct/mct-syscall.h"
#include "targets.hpp"

import sys;

unsigned clean_level{};

static void usage() {
  die(R"(
  Usage: ../leco/leco.exe [-c [-c]] [-C <dir>] [-g] [-O] [-t <target>] [<tool> [<tool options>]]

  Where:
    -c -- clean current module before build (if repeated, clean all modules)

    -C -- change to this directory before build

    -g -- enable debug symbols

    -O -- enable optimisations

    -t <target> -- one of:
      iphoneos, iphonesimulator:
          for its referring platform (requires Apple SDKs and an Apple host)
      android_aarch64, android_armv7, android_x86, android_x86_64:
          for each of four Android architectures (requires Android SDK on any host)
      wasm:
          for WebAssembly (requires WASI sysroot and clang's built-in runtime)
      if abscent, uses host target

    <tool> -- tool name to run (runs the whole build if absent)

      clang: runs clang with opiniated defaults
      clean: cleans output folders (same as "-c")
      codesign: runs codesign on Apple platforms
      dagger: generates dependency files based on parsed code
      deplist: creates a list of required modules and includes
      driver: drives the workflow (default tool if unspecified)
      exs: copies dependent executables to app bundles
      ipa-export: exports an IPA archive
      ipa-upload: validates/upload iOS binaries
      ipa: generates and signs IPAs
      link: links an executable
      rc: generates "RC" files for Windows
      rsrc: copies resource files to their app's resource folder
      shaders: compiles GLSL into SPIRV
      sysroot: calculates the sysroot of the environment
      wasm-js: almost a webpacker
      xcassets: generates xcassets for IPAs
)");
}

static void run_target() {
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
  if (target == "iphoneos") return TGT_IPHONEOS;
  if (target == "iphonesimulator") return TGT_IOS_SIMULATOR;
#endif

  if (target == "wasm") return TGT_WASM;

  if (target == "android_aarch64") return TGT_DROID_AARCH64;
  if (target == "android_armv7")   return TGT_DROID_ARMV7;
  if (target == "android_x86")     return TGT_DROID_X86;
  if (target == "android_x86_64")  return TGT_DROID_X86_64;

  dief("unknown or invalid target for this platform: %s", *target);
}

static int run_tool(int argc, char ** argv) {
  if (argv[argc]) die("argv does not end with a nullptr");

  auto cmd = sys::tool_cmd(argv[0]);
  argv[0] = *cmd;
  return mct_syscall_spawn(argv[0], argv);
}

static void chdir(const char * dir) {
  if (0 != mct_syscall_chdir(dir)) dief("Directory not found: [%s]", dir);
}

int main(int argc, char ** argv) try {
  const auto shift = [&] { return argc > 1 ? (argc--, *++argv) : nullptr; };
  while (auto val = shift()) {
    if ("-c"_s == val) clean_level++;
    else if ("-C"_s == val) chdir(shift());
    else if ("-t"_s == val) mct_syscall_setenv("LECO_TARGET", target_of(shift()));
    else if ("-g"_s == val) mct_syscall_setenv("LECO_DEBUG", "1");
    else if ("-O"_s == val) mct_syscall_setenv("LECO_OPT", "1");
    else if (mtime::of(*sys::tool_cmd(val))) return run_tool(argc, argv);
    else usage();
  }

  run_target();
  return 0;
} catch (...) {
  return 1;
}
