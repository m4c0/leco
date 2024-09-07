#pragma leco tool

#include "sim.hpp"
#include "targets.hpp"

#include <stdio.h>
#include <string.h>

import gopt;
import mtime;
import pprent;
import sys;

void usage() { sys::die("invalid usage"); }

static bool exists(const sim_sb *path) { return mtime::of(path->buffer) > 0; }

static void find_android_llvm(sim_sb *res) {
  const auto sdk = sys::env("ANDROID_SDK_ROOT");
  if (sdk == nullptr)
    sys::die("undefined ANDROID_SDK_ROOT");

  sim_sb_path_copy_append(res, sdk, "ndk-bundle");
  if (exists(res))
    return;

  sim_sb_path_copy_append(res, sdk, "ndk");
  if (!exists(res))
    sys::die("ANDROID_SDK_ROOT path does not contain a folder named 'sdk': [%s]",
        res->buffer);

  sim_sbt max{};
  sim_sb_copy(&max, "");

  for (auto e : pprent::list(res->buffer)) {
    if (strcmp(max.buffer, e) > 0)
      continue;

    sim_sb_copy(&max, e);
  }

  sim_sb_path_append(res, max.buffer);
  sim_sb_path_append(res, "toolchains");
  sim_sb_path_append(res, "llvm");
  sim_sb_path_append(res, "prebuilt");
  if (!exists(res))
    sys::die("prebuilt path isn't a directory: [%s]", res->buffer);

  for (auto e : pprent::list(res->buffer)) {
    if (e[0] == '.')
      continue;

    sim_sb_path_append(res, e);
    return;
  }
  sys::die("no LLVM inside prebuilt dir: [%s]", res->buffer);
}

static const char *apple_sysroot(const char *sdk) {
#ifndef __APPLE__
  sys::die("apple targets not supported when host isn't osx");
  return nullptr;
#else
  sim_sbt cmd{};
  sim_sb_printf(&cmd, "xcrun --show-sdk-path --sdk %s", sdk);

  static char buf[256];

  auto f = popen(cmd.buffer, "r");
  auto path = fgets(buf, sizeof(buf), f);
  pclose(f);

  if (path == nullptr)
    return nullptr;

  path[strlen(path) - 1] = 0; // chomp "\n"
  return path;
#endif
}

static const char *android_sysroot() {
  sim_sbt llvm{};
  find_android_llvm(&llvm);
  sim_sb_path_append(&llvm, "sysroot");
  return nullptr;
}

static const char *wasm_sysroot() {
  const auto sdk = sys::env("WASI_SYSROOT");
  if (sdk == nullptr)
    sys::die("undefined WASI_SYSROOT");

  return sdk;
}

static const char *sysroot_for_target(const char *target) {
  if (IS_TGT(target, TGT_OSX)) {
    return apple_sysroot("macosx");
  } else if (IS_TGT(target, TGT_IPHONEOS)) {
    return apple_sysroot("iphoneos");
  } else if (IS_TGT(target, TGT_IOS_SIMULATOR)) {
    return apple_sysroot("iphonesimulator");
  }

  if (IS_TGT_DROID(target)) {
    return android_sysroot();
  }
  if (IS_TGT(target, TGT_WASM)) {
    return wasm_sysroot();
  }

  return nullptr;
}

int main(int argc, char **argv) try {
  const char *target{HOST_TARGET};
  bool quiet{};
  auto opts = gopt_parse(argc, argv, "qt:", [&](auto ch, auto val) {
    switch (ch) {
    case 't':
      target = val;
      break;
    case 'q':
      quiet = true;
      break;
    default:
      usage();
      break;
    }
  });
  if (opts.argc > 0)
    usage();

  sim_sbt cf{};
  sim_sb_path_copy_real(&cf, argv[0]);
  sim_sb_path_parent(&cf);
  sim_sb_path_parent(&cf);
  sim_sb_path_append(&cf, target);
  sys::mkdirs(cf.buffer);
  sim_sb_path_append(&cf, "sysroot");
  if (mtime::of(cf.buffer) > 0) {
    auto f = sys::fopen(cf.buffer, "r");
    sim_sbt buf{};
    if (fgets(buf.buffer, buf.size, f) != nullptr) {
      if (!quiet)
        fwrite(buf.buffer, 1, buf.size, stdout);
      return 0;
    }
    fclose(f);
  }

  auto sysroot = sysroot_for_target(target);
  if (sysroot) {
    if (!quiet)
      puts(sysroot);

    auto f = sys::fopen(cf.buffer, "w");
    fputs(sysroot, f);
    fclose(f);
  }
} catch (...) {
  return 1;
}
