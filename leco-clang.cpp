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
  sys::for_each_target_def([&](auto d) {
    *buf += " -D";
    *buf += d;
  });
  if (sys::is_tgt_apple()) {
    *buf += " -D_C99_SOURCE";
  } else if (sys::is_tgt_wasm()) {
    *buf += " -fwasm-exceptions";
  }
}

static void add_sysroot(sim::sb * args, const char * argv0) {
  auto sra = sim::path_parent(argv0).path_parent() / sys::target() / "sysroot";
  if (!mtime::of(*sra)) return; // Optional in WASM and host targets

  args->printf(" --sysroot @%s", *sra);
}

int main(int argc, char **argv) try {
  if (argc == 1) usage();

  auto args = sim::sb { CLANG_CMD } + " -Wall -Wno-unknown-pragmas";

  if (sys::is_debug()) args += sys::is_tgt_windows() ? " -gdwarf" : " -g";
  if (sys::is_opt())   args += " -O3 -flto -fvisibility=hidden";

  args.printf(" -target %s", (const char *)sys::target());
  add_target_defs(&args);
  add_sysroot(&args, argv[0]);

  // TODO: escape argv or use exec
  for (auto i = 1; i < argc; i++) args.printf(" %s", argv[i]);

  sys::run(*args);
  return 0;
} catch (...) {
  return 1;
}
