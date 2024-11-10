#pragma leco tool

#include "targets.hpp"

#include <stdio.h>
#include <string.h>

import gopt;
import mtime;
import pprent;
import sim;
import sys;

void usage() { sys::die("invalid usage"); }

static bool exists(const char * path) { return mtime::of(path) > 0; }

static sim::sb android_sysroot() {
  const sim::sb sdk { sys::env("ANDROID_SDK_ROOT") };

  auto res = sdk / "ndk-bundle";
  if (exists(*res)) return res;

  res = sdk / "ndk";
  if (!exists(*res))
    sys::die("ANDROID_SDK_ROOT path does not contain a folder named: [%s]", *res);

  sim::sb max { "" };

  for (auto e : pprent::list(*res)) {
    if (strcmp(*max, e) > 0) continue;
    max = sim::sb { e };
  }

  res = res / *max / "toolchains" / "llvm" / "prebuilt";
  if (!exists(*res)) sys::die("prebuilt path isn't a directory: [%s]", *res);

  for (auto e : pprent::list(*res)) {
    if (e[0] == '.') continue;

    return res / e / "sysroot";
  }
  sys::die("no LLVM inside prebuilt dir: [%s]", *res);
}

static sim::sb apple_sysroot(const char *sdk) {
#ifndef __APPLE__
  sys::die("apple targets not supported when host isn't osx");
  return {};
#else
  auto cmd = sim::printf("xcrun --show-sdk-path --sdk %s", sdk);

  sim::sb buf {};

  auto f = popen(*cmd, "r");
  auto path = fgets(*buf, buf.len, f);
  pclose(f);

  if (path == nullptr) return {};

  (*buf)[--buf.len] = 0; // chomp "\n"
  return buf;
#endif
}

static sim::sb wasm_sysroot() { return sim::sb { sys::env("WASI_SYSROOT") }; }

static sim::sb sysroot_for_target(const char *target) {
  if (IS_TGT(target, TGT_OSX)) return apple_sysroot("macosx");
  if (IS_TGT(target, TGT_IPHONEOS)) return apple_sysroot("iphoneos");
  if (IS_TGT(target, TGT_IOS_SIMULATOR)) return apple_sysroot("iphonesimulator");
  if (IS_TGT_DROID(target)) return android_sysroot();
  if (IS_TGT(target, TGT_WASM)) return wasm_sysroot();
  return {};
}

int main(int argc, char **argv) try {
  const char *target{HOST_TARGET};
  bool quiet{};
  auto opts = gopt_parse(argc, argv, "qt:", [&](auto ch, auto val) {
    switch (ch) {
      case 't': target = val; break;
      case 'q': quiet = true; break;
      default: usage(); break;
    }
  });
  if (opts.argc > 0) usage();

  auto cf = sim::path_real(argv[0]);
  cf.path_parent();
  cf.path_parent();
  cf /= target;
  sys::mkdirs(*cf);
  cf /= "sysroot";
  if (exists(*cf)) {
    auto f = sys::fopen(*cf, "r");
    char buf[10240] {};
    if (fgets(buf, sizeof(buf), f) != nullptr) {
      if (!quiet) fwrite(buf, 1, sizeof(buf), stdout);
      return 0;
    }
    fclose(f);
  }

  auto sysroot = sysroot_for_target(target);
  if (sysroot.len) {
    if (!quiet) puts(*sysroot);

    auto f = sys::fopen(*cf, "w");
    fputs(*sysroot, f);
    fclose(f);
  }
} catch (...) {
  return 1;
}
