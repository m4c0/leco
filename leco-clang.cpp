#pragma leco tool
#define SIM_IMPLEMENTATION
#define SYSSTD_IMPLEMENTATION

#include "../sysstd/sysstd.h"
#include "sim.h"
#include "targets.hpp"

#define IS_TGT(t, x) (0 == strcmp((t), (x)))
#define IS_TGT_DROID(t)                                                        \
  (IS_TGT(t, TGT_DROID_AARCH64) || IS_TGT(t, TGT_DROID_ARMV7) ||               \
   IS_TGT(t, TGT_DROID_X86) || IS_TGT(t, TGT_DROID_X86_64))

static const char * clang_cmd() {
#if __APPLE__ && !__arm64__
  return "/usr/local/opt/llvm/bin/clang++";
#elif __APPLE__
  return "/opt/homebrew/opt/llvm/bin/clang++";
#elif _WIN32
  return "clang++.exe";
#else
  return "clang++";
#endif
}

static void run(const char * cmd) {
  if (0 == system(cmd)) return;
  fprintf(stderr, "command failed: %s\n", cmd);
  throw 0;
}

static int usage() {
  fprintf(stderr, R"(
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
  return 1;
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
    fprintf(stderr, "invalid target: [%s]\n", tgt);
    throw 0;
  }
}

static void add_sysroot(sim_sb * args, const char * target, const char * argv0) {
  sim_sb sra{};
  sim_sb_new(&sra, 10240);
  sim_sb_path_copy_parent(&sra, argv0);
  sim_sb_path_parent(&sra);
  sim_sb_path_append(&sra, target);
  sim_sb_path_append(&sra, "sysroot");

  auto f = sysstd_fopen(sra.buffer, "r");
  if (!f) return;
  fgets(sra.buffer, sra.size, f);
  fclose(f);

  sim_sb_printf(args, " --sysroot %s", sra.buffer);
}

int main(int argc, char **argv) try {
  if (argc == 1) usage();

  const char * target = sysstd_env("LECO_TARGET");
  if (!target) target = HOST_TARGET;

  sim_sb args{};
  sim_sb_new(&args, 10240);
  sim_sb_copy(&args, clang_cmd());
  sim_sb_concat(&args, " -Wall -Wno-unknown-pragmas");

  if (sysstd_env("LECO_DEBUG")) {
#ifdef _WIN32
    sim_sb_concat(&args, " -gdwarf");
#else
    sim_sb_concat(&args, " -g");
#endif
  }
  if (sysstd_env("LECO_OPT")) sim_sb_concat(&args, " -O3 -flto -fvisibility=hidden");

  if (0 == strcmp(target, TGT_WASM)) sim_sb_concat(&args, " -fwasm-exceptions");

  sim_sb_printf(&args, " -target %s", target);
  add_target_defs(&args, target);
  if (0 != strcmp(target, HOST_TARGET)) add_sysroot(&args, target, argv[0]);

  // TODO: escape argv or use exec
  for (auto i = 1; i < argc; i++) sim_sb_printf(&args, " %s", argv[i]);

  run(args.buffer);
  return 0;
} catch (...) {
  return 1;
}
