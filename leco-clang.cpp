#pragma leco tool
#include <stdio.h>
#include <string.h>
#include "sim.h"
#include "targets.hpp"

import sys;

#define IS_TGT(t, x) (0 == strcmp((t), (x)))
#define IS_TGT_DROID(t)                                                        \
  (IS_TGT(t, TGT_DROID_AARCH64) || IS_TGT(t, TGT_DROID_ARMV7) ||               \
   IS_TGT(t, TGT_DROID_X86) || IS_TGT(t, TGT_DROID_X86_64))

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

static void add_target_defs(sim_sb *buf, const char *tgt) {
  if (IS_TGT(tgt, TGT_WINDOWS)) {
    sim_sb_concat(buf, " -DLECO_TARGET_WINDOWS");
  } else if (IS_TGT(tgt, TGT_LINUX)) {
    sim_sb_concat(buf, " -DLECO_TARGET_LINUX");
  } else if (IS_TGT(tgt, TGT_OSX)) {
    sim_sb_concat(buf, " -DLECO_TARGET_MACOSX");
    sim_sb_concat(buf, " -DLECO_TARGET_APPLE");
    sim_sb_concat(buf, " -D_C99_SOURCE");
  } else if (IS_TGT(tgt, TGT_IPHONEOS)) {
    sim_sb_concat(buf, " -DLECO_TARGET_IPHONEOS");
    sim_sb_concat(buf, " -DLECO_TARGET_IOS");
    sim_sb_concat(buf, " -DLECO_TARGET_APPLE");
    sim_sb_concat(buf, " -D_C99_SOURCE");
  } else if (IS_TGT(tgt, TGT_IOS_SIMULATOR)) {
    sim_sb_concat(buf, " -DLECO_TARGET_IPHONESIMULATOR");
    sim_sb_concat(buf, " -DLECO_TARGET_IOS");
    sim_sb_concat(buf, " -DLECO_TARGET_APPLE");
    sim_sb_concat(buf, " -D_C99_SOURCE");
  } else if (IS_TGT_DROID(tgt)) {
    sim_sb_concat(buf, " -DLECO_TARGET_ANDROID");
  } else if (IS_TGT(tgt, TGT_WASM)) {
    sim_sb_concat(buf, " -DLECO_TARGET_WASM");
  } else {
    die("invalid target: ", tgt);
  }
}

static void add_sysroot(sim_sb * args, const char * target, const char * argv0) {
  sim_sb sra{};
  sim_sb_new(&sra, 10240);
  sim_sb_path_copy_parent(&sra, argv0);
  sim_sb_path_parent(&sra);
  sim_sb_path_append(&sra, target);
  sim_sb_path_append(&sra, "sysroot");

  sys::file f { sra.buffer, "r" };
  fgets(sra.buffer, sra.size, f);

  sim_sb_printf(args, " --sysroot %s", sra.buffer);
}

int main(int argc, char **argv) try {
  if (argc == 1) usage();

  auto target = sys::target();

  sim_sb args{};
  sim_sb_new(&args, 10240);
  sim_sb_copy(&args, CLANG_CMD);
  sim_sb_concat(&args, " -Wall -Wno-unknown-pragmas");

  if ((const char *)sys::opt_envs::debug()) {
#ifdef _WIN32
    sim_sb_concat(&args, " -gdwarf");
#else
    sim_sb_concat(&args, " -g");
#endif
  }
  if ((const char *)sys::opt_envs::opt()) sim_sb_concat(&args, " -O3 -flto -fvisibility=hidden");

  if (0 == strcmp(target, TGT_WASM)) sim_sb_concat(&args, " -fwasm-exceptions");

  sim_sb_printf(&args, " -target %s", (const char *)target);
  add_target_defs(&args, target);
  if (0 != strcmp(target, HOST_TARGET)) add_sysroot(&args, target, argv[0]);

  // TODO: escape argv or use exec
  for (auto i = 1; i < argc; i++) sim_sb_printf(&args, " %s", argv[i]);

  sys::run(args.buffer);
  return 0;
} catch (...) {
  return 1;
}
