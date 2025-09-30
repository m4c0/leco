#pragma leco tool

#include <stdio.h>
#include <string.h>

import popen;
import sys;

static bool exists(const char * path) { return mtime::of(path) > 0; }

static sim::sb android_sysroot() {
  const sim::sb sdk { sys::envs::android_sdk_root() };

  auto res = sdk / "ndk-bundle";
  if (exists(*res)) return res;

  res = sdk / "ndk";
  if (!exists(*res))
    dief("ANDROID_SDK_ROOT path does not contain a folder named: [%s]", *res);

  sim::sb max { "" };

  for (auto e : pprent::list(*res)) {
    if (strcmp(*max, e) > 0) continue;
    max = sim::sb { e };
  }

  res = res / *max / "toolchains" / "llvm" / "prebuilt";
  if (!exists(*res)) dief("prebuilt path isn't a directory: [%s]", *res);

  for (auto e : pprent::list(*res)) {
    if (e[0] == '.') continue;

    return res / e / "sysroot";
  }
  dief("no LLVM inside prebuilt dir: [%s]", *res);
}

static sim::sb apple_sysroot(const char *sdk) {
#ifndef __APPLE__
  die("apple targets not supported when host isn't osx");
  return {};
#else
  auto p = p::proc { "xcrun", "--show-sdk-path", "--sdk", sdk };
  if (!p.gets()) return {};
  return sim::sb { p.last_line_read() };
#endif
}

static sim::sb wasm_sysroot() {
  auto e = sys::opt_envs::wasi_sysroot();
  return (const char *)e ? sim::sb { e } : sim::sb {};
}

static sim::sb sysroot_for_target() {
  if (sys::is_tgt_osx())      return apple_sysroot("macosx");
  if (sys::is_tgt_iphoneos()) return apple_sysroot("iphoneos");
  if (sys::is_tgt_ios_sim())  return apple_sysroot("iphonesimulator");
  if (sys::is_tgt_droid())    return android_sysroot();
  if (sys::is_tgt_wasm())     return wasm_sysroot();
  dief("invalid target: %s", (const char *)sys::target());
}

int main() try {
  if (sys::is_tgt_host()) return 0;

  auto cf = sim::printf("../leco/out/%s", (const char *)sys::target());
  sys::mkdirs(*cf);
  cf /= "sysroot";
  if (exists(*cf)) return 0; 

  auto sysroot = sysroot_for_target();
  if (!sysroot.len) return 0;

  sys::log("sysroot", *sysroot);
  fputs(*sysroot, sys::file { *cf, "w" });
} catch (...) {
  return 1;
}
