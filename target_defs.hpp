#pragma once
#include "context.hpp"
#include "droid_path.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

namespace t {
static auto macosx() {
  return context{
      .target = "x86_64-apple-macosx11.6.0",
      .native_target = true,
  };
}
static auto iphoneos() {
  return context{
      .target = "arm64-apple-ios13.0",
  };
}
static auto iphonesimulator() {
  return context{
      .target = "x86_64-apple-ios13.0-simulator",
  };
}

static auto windows() {
  return context{
      .target = "x86_64-pc-windows-msvc",
      .native_target = true,
  };
}

static auto android(llvm::StringRef tgt) {
  llvm::SmallString<256> llvm{};
  find_android_llvm(llvm);
  llvm::sys::path::append(llvm, "sysroot");
  return context{
      .target = tgt.str(),
      .sysroot = llvm.str().str(),
  };
}
static auto android_aarch64() {
  return android("aarch64-none-linux-android26");
}
static auto android_armv7() {
  return android("armv7-none-linux-androideabi26");
}
static auto android_i686() { return android("i686-none-linux-android26"); }
static auto android_x86_64() { return android("x86_64-none-linux-android26"); }
} // namespace t
