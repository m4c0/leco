#pragma leco tool

#include <stdio.h>
#include <string.h>

import popen;
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


  auto p = p::proc(*cmd, "r");
  if (!p.gets()) return {};
  return sim::sb { p.last_line_read() }.chomp();
#endif
}

static sim::sb wasm_sysroot() { return sim::sb { sys::env("WASI_SYSROOT") }; }

static sim::sb sysroot_for_target() {
  if (sys::is_tgt_osx())      return apple_sysroot("macosx");
  if (sys::is_tgt_iphoneos()) return apple_sysroot("iphoneos");
  if (sys::is_tgt_ios_sim())  return apple_sysroot("iphonesimulator");
  if (sys::is_tgt_droid())    return android_sysroot();
  if (sys::is_tgt_wasm())     return wasm_sysroot();
  sys::die("invalid target: %s", sys::target());
}

int main() try {
  if (sys::is_tgt_host()) return 0;

  auto cf = sim::printf("../leco/out/%s", sys::target());
  sys::mkdirs(*cf);
  cf /= "sysroot";
  if (exists(*cf)) return 0; 

  auto sysroot = sysroot_for_target();
  if (!sysroot.len) return 0;

  fputs(*sysroot, sys::file { *cf, "w" });
} catch (...) {
  return 1;
}
