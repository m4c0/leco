#pragma once
#include "context.hpp"
#include "droid_path.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

namespace t::impl {
static auto android(llvm::StringRef tgt) {
  static llvm::SmallVector<llvm::StringRef, 3> predefs{{
      "LECO_TARGET_ANDROID",
  }};
  llvm::SmallString<256> llvm{};
  find_android_llvm(llvm);
  llvm::sys::path::append(llvm, "sysroot");
  return context{
      .target = tgt.str(),
      .sysroot = llvm.str().str(),
      .predefs = predefs,
  };
}
} // namespace t::impl
namespace t {
static auto macosx() {
  static llvm::SmallVector<llvm::StringRef, 2> predefs{{
      "LECO_TARGET_MACOSX",
      "LECO_TARGET_APPLE",
  }};
  return context{
      .target = "x86_64-apple-macosx11.6.0",
      .native_target = true,
      .predefs = predefs,
  };
}
static auto iphoneos() {
  static llvm::SmallVector<llvm::StringRef, 3> predefs{{
      "LECO_TARGET_IPHONEOS",
      "LECO_TARGET_IOS",
      "LECO_TARGET_APPLE",
  }};
  return context{
      .target = "arm64-apple-ios13.0",
      .predefs = predefs,
  };
}
static auto iphonesimulator() {
  static llvm::SmallVector<llvm::StringRef, 3> predefs{{
      "LECO_TARGET_IPHONESIMULATOR",
      "LECO_TARGET_IOS",
      "LECO_TARGET_APPLE",
  }};
  return context{
      .target = "x86_64-apple-ios13.0-simulator",
      .predefs = predefs,
  };
}

static auto windows() {
  static llvm::SmallVector<llvm::StringRef, 3> predefs{{
      "LECO_TARGET_WINDOWS",
  }};
  return context{
      .target = "x86_64-pc-windows-msvc",
      .native_target = true,
      .predefs = predefs,
  };
}

static auto android_aarch64() {
  return impl::android("aarch64-none-linux-android26");
}
static auto android_armv7() {
  return impl::android("armv7-none-linux-androideabi26");
}
static auto android_i686() {
  return impl::android("i686-none-linux-android26");
}
static auto android_x86_64() {
  return impl::android("x86_64-none-linux-android26");
}
} // namespace t
