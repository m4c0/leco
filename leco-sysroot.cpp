#pragma leco tool

#include <stdio.h>
#include <string.h>

import gopt;
import mtime;
import pprent;
import sim;
import sys;

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
  auto path = fgets(*buf, buf.size, f);
  pclose(f);

  if (path == nullptr) return {};

  buf.len = strlen(path) - 1;
  (*buf)[buf.len] = 0; // chomp "\n"
  return buf;
#endif
}

static sim::sb wasm_sysroot() { return sim::sb { sys::env("WASI_SYSROOT") }; }

static sim::sb sysroot_for_target(const char *target) {
  if (sys::is_tgt_osx(target))      return apple_sysroot("macosx");
  if (sys::is_tgt_iphoneos(target)) return apple_sysroot("iphoneos");
  if (sys::is_tgt_ios_sim(target))  return apple_sysroot("iphonesimulator");
  if (sys::is_tgt_droid(target))    return android_sysroot();
  if (sys::is_tgt_wasm(target))     return wasm_sysroot();
  sys::die("invalid target: %s", target);
}

int main(int argc, char **argv) try {
  const char * target = sys::target();
  if (sys::is_tgt_host(target)) return 0;

  auto cf = sim::path_real(argv[0]);
  cf.path_parent();
  cf.path_parent();
  cf /= target;
  sys::mkdirs(*cf);
  cf /= "sysroot";
  if (exists(*cf)) return 0; 

  auto sysroot = sysroot_for_target(target);
  if (!sysroot.len) return 0;

  auto f = sys::fopen(*cf, "w");
  fputs(*sysroot, f);
  fclose(f);
} catch (...) {
  return 1;
}
