#include "context.hpp"
#include "droid_path.hpp"
#include "plist.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

using namespace llvm;

namespace t::impl {
context android(llvm::StringRef tgt) {
  static llvm::SmallVector<llvm::StringRef, 3> predefs{{
      "LECO_TARGET_ANDROID",
  }};
  llvm::SmallString<256> llvm{};
  find_android_llvm(llvm);
  llvm::sys::path::append(llvm, "sysroot");
  return context{
      .predefs = predefs,
      .target = tgt.str(),
      .sysroot = llvm.str().str(),
      .app_exe_path = [](auto exe, auto stem) {},
      .app_res_path = [](auto exe) {},
      .bundle = [](auto &exe, auto stem) {},
  };
}
static std::string apple_sysroot(llvm::StringRef sdk) {
#ifdef __APPLE__
  constexpr const auto limit = 256;
  llvm::SmallString<limit> buf{};

  buf.assign("xcrun --show-sdk-path --sdk ");
  buf.append(sdk);

  auto f = popen(buf.c_str(), "r");
  auto path = fgets(buf.data(), limit, f);
  pclose(f);

  if (path == nullptr)
    return "";

  path[strlen(path) - 1] = 0; // chomp "\n"
  return path;
#else
  return "";
#endif
}
void apple_bundle_path(llvm::SmallVectorImpl<char> &exe, llvm::StringRef stem) {
  llvm::sys::path::remove_filename(exe);
  llvm::sys::path::append(exe, stem);
  llvm::sys::path::replace_extension(exe, "app");
}
} // namespace t::impl
namespace t {
context macosx() {
  static llvm::SmallVector<llvm::StringRef, 2> predefs{{
      "LECO_TARGET_MACOSX",
      "LECO_TARGET_APPLE",
  }};
  return context{
      .predefs = predefs,
      .target = "x86_64-apple-macosx11.6.0",
      .sysroot = impl::apple_sysroot("macosx"),
      .app_exe_path =
          [](auto &exe, auto stem) {
            impl::apple_bundle_path(exe, stem);
            llvm::sys::path::append(exe, "Contents", "MacOS");
            llvm::sys::path::append(exe, stem);
          },
      .app_res_path =
          [](auto exe) {
            llvm::sys::path::remove_filename(exe);
            llvm::sys::path::remove_filename(exe);
            llvm::sys::path::append(exe, "Resources");
          },
      .bundle = [](auto &exe, auto stem) {},
      .native_target = true,
  };
}
context iphoneos() {
  static llvm::SmallVector<llvm::StringRef, 3> predefs{{
      "LECO_TARGET_IPHONEOS",
      "LECO_TARGET_IOS",
      "LECO_TARGET_APPLE",
  }};
  return context{
      .predefs = predefs,
      .target = "arm64-apple-ios13.0",
      .sysroot = impl::apple_sysroot("iphoneos"),
      .app_exe_path =
          [](auto &exe, auto stem) {
            impl::apple_bundle_path(exe, stem);
            sys::path::append(exe, stem);
          },
      .app_res_path = [](auto exe) { sys::path::remove_filename(exe); },
      .bundle =
          [](auto &exe, auto stem) {
            impl::apple_bundle_path(exe, stem);

            auto b_path = StringRef{exe.begin(), exe.size()};
            gen_info_plist(b_path, stem);
            gen_archive_plist(sys::path::parent_path(b_path), stem);
            gen_export_plist(sys::path::parent_path(b_path), stem);
          },
  };
}
context iphonesimulator() {
  static llvm::SmallVector<llvm::StringRef, 3> predefs{{
      "LECO_TARGET_IPHONESIMULATOR",
      "LECO_TARGET_IOS",
      "LECO_TARGET_APPLE",
  }};
  return context{
      .predefs = predefs,
      .target = "x86_64-apple-ios13.0-simulator",
      .sysroot = impl::apple_sysroot("iphonesimulator"),
      .app_exe_path =
          [](auto &exe, auto stem) {
            impl::apple_bundle_path(exe, stem);
            llvm::sys::path::append(exe, stem);
          },
      .app_res_path = [](auto exe) { sys::path::remove_filename(exe); },
      .bundle = [](auto &exe, auto stem) {},
  };
}

context windows() {
  static llvm::SmallVector<llvm::StringRef, 3> predefs{{
      "LECO_TARGET_WINDOWS",
  }};
  return context{
      .predefs = predefs,
      .target = "x86_64-pc-windows-msvc",
      .app_exe_path =
          [](auto exe, auto stem) {
            impl::apple_bundle_path(exe, stem);
            llvm::sys::path::append(exe, stem);
            llvm::sys::path::replace_extension(exe, "exe");
          },
      .app_res_path = [](auto exe) { sys::path::remove_filename(exe); },
      .bundle = [](auto &exe, auto stem) {},
      .native_target = true,
  };
}

context android_aarch64() {
  return impl::android("aarch64-none-linux-android26");
}
context android_armv7() {
  return impl::android("armv7-none-linux-androideabi26");
}
context android_i686() { return impl::android("i686-none-linux-android26"); }
context android_x86_64() {
  return impl::android("x86_64-none-linux-android26");
}
} // namespace t
