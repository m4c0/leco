#pragma leco tool
#include "targets.hpp"

import sys;

static void usage() {
  die(R"(
LECO's heavily-opiniated CLANG runner

Usage: ../leco/leco.exe clang <clang-flags>

This tool uses the clang version available via PATH, except on MacOS where it 
requires llvm to be installed via Homebrew.

Where:
      <clang-flags>  pass flags as-is to clang

Environment variables:
      LECO_TARGET    target triple (check this tool's source for list of
                     supported targets)

      LECO_DEBUG     enable debug flags

      LECO_OPT       enable optimisation flags

)");
}

static void add_target_defs(sim::sb * buf) {
  if (sys::is_tgt_windows()) {
    *buf += " -DLECO_TARGET_WINDOWS";
  } else if (sys::is_tgt_linux()) {
    *buf += " -DLECO_TARGET_LINUX";
  } else if (sys::is_tgt_osx()) {
    *buf += " -DLECO_TARGET_MACOSX";
    *buf += " -DLECO_TARGET_APPLE";
    *buf += " -D_C99_SOURCE";
  } else if (sys::is_tgt_iphoneos()) {
    *buf += " -DLECO_TARGET_IPHONEOS";
    *buf += " -DLECO_TARGET_IOS";
    *buf += " -DLECO_TARGET_APPLE";
    *buf += " -D_C99_SOURCE";
  } else if (sys::is_tgt_ios_sim()) {
    *buf += " -DLECO_TARGET_IPHONESIMULATOR";
    *buf += " -DLECO_TARGET_IOS";
    *buf += " -DLECO_TARGET_APPLE";
    *buf += " -D_C99_SOURCE";
  } else if (sys::is_tgt_droid()) {
    *buf += " -DLECO_TARGET_ANDROID";
  } else if (sys::is_tgt_wasm()) {
    *buf += " -DLECO_TARGET_WASM";
  } else {
    die("invalid target: ", (const char *)sys::target());
  }
}

static void add_sysroot(sim::sb * args, const char * argv0) {
  auto sra = sim::path_parent(argv0).path_parent() / sys::target() / "sysroot";
  if (!mtime::of(*sra)) return; // Optional in WASM

  args->printf(" --sysroot @%s", *sra);
}

int main(int argc, char **argv) try {
  if (argc == 1) usage();

  auto args = sim::sb { CLANG_CMD } + " -Wall -Wno-unknown-pragmas";

  if ((const char *)sys::opt_envs::debug()) {
#ifdef _WIN32
    args += " -gdwarf";
#else
    args += " -g";
#endif
  }
  if ((const char *)sys::opt_envs::opt()) args += " -O3 -flto -fvisibility=hidden";

  if (sys::is_tgt_wasm()) args += " -fwasm-exceptions";

  args.printf(" -target %s", (const char *)sys::target());
  add_target_defs(&args);
  if (!sys::is_tgt_host()) add_sysroot(&args, argv[0]);

  // TODO: escape argv or use exec
  for (auto i = 1; i < argc; i++) args.printf(" %s", argv[i]);

  sys::run(*args);
  return 0;
} catch (...) {
  return 1;
}
