#include "actool.hpp"
#include "context.hpp"
#include "droid_path.hpp"
#include "plist.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

using namespace llvm;

namespace t::impl {
context android(StringRef tgt) {
  static SmallVector<StringRef, 3> predefs{{
      "LECO_TARGET_ANDROID",
  }};
  SmallString<256> llvm{};
  find_android_llvm(llvm);
  sys::path::append(llvm, "sysroot");
  return context{
      .predefs = predefs,
      .target = tgt.str(),
      .sysroot = llvm.str().str(),
      .app_exe_path = [](auto exe, auto stem) {},
      .app_res_path = [](auto exe) {},
      .bundle = [](auto &exe, auto stem) {},
  };
}
static std::string apple_sysroot(StringRef sdk) {
#ifdef __APPLE__
  constexpr const auto limit = 256;
  SmallString<limit> buf{};

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
llvm::ArrayRef<llvm::StringRef> macos_link_flags() {
  static SmallVector<StringRef, 3> flags{{
      "-rpath",
      "@executable_path",
  }};
  return flags;
}
llvm::ArrayRef<llvm::StringRef> ios_link_flags() {
  static SmallVector<StringRef, 3> flags{{
      "-rpath",
      "@executable_path/Frameworks",
  }};
  return flags;
}
void apple_bundle_path(SmallVectorImpl<char> &exe, StringRef stem) {
  sys::path::remove_filename(exe);
  sys::path::append(exe, stem);
  sys::path::replace_extension(exe, "app");
}
} // namespace t::impl
namespace t {
context macosx() {
  static SmallVector<StringRef, 2> predefs{{
      "LECO_TARGET_MACOSX",
      "LECO_TARGET_APPLE",
  }};
  return context{
      .predefs = predefs,
      .link_flags = impl::macos_link_flags(),
      .target = "x86_64-apple-macosx11.6.0",
      .sysroot = impl::apple_sysroot("macosx"),
      .app_exe_path =
          [](auto &exe, auto stem) {
            impl::apple_bundle_path(exe, stem);
            sys::path::append(exe, "Contents", "MacOS");
            sys::path::append(exe, stem);
            sys::path::replace_extension(exe, "exe");
          },
      .app_res_path =
          [](auto exe) {
            sys::path::remove_filename(exe);
            sys::path::remove_filename(exe);
            sys::path::append(exe, "Resources");
          },
      .bundle = [](auto &exe, auto stem) {},
      .native_target = true,
  };
}
context iphoneos() {
  static SmallVector<StringRef, 3> predefs{{
      "LECO_TARGET_IPHONEOS",
      "LECO_TARGET_IOS",
      "LECO_TARGET_APPLE",
  }};
  return context{
      .predefs = predefs,
      .link_flags = impl::ios_link_flags(),
      .target = "arm64-apple-ios16.1",
      .sysroot = impl::apple_sysroot("iphoneos"),
      .rpath = "Frameworks",
      .app_exe_path =
          [](auto &exe, auto stem) {
            sys::path::remove_filename(exe);
            sys::path::append(exe, "export.xcarchive", "Products",
                              "Applications", stem);
            sys::path::replace_extension(exe, "app");

            sys::path::append(exe, stem);
          },
      .app_res_path = [](auto exe) { sys::path::remove_filename(exe); },
      .bundle =
          [](auto &exe, auto stem) {
            auto b_path = StringRef{exe.begin(), exe.size()};
            if (!actool(b_path))
              return;
            gen_iphone_plists(b_path, stem);
          },
  };
}
context iphonesimulator() {
  static SmallVector<StringRef, 3> predefs{{
      "LECO_TARGET_IPHONESIMULATOR",
      "LECO_TARGET_IOS",
      "LECO_TARGET_APPLE",
  }};
  return context{
      .predefs = predefs,
      .link_flags = impl::ios_link_flags(),
      .target = "x86_64-apple-ios16.1-simulator",
      .sysroot = impl::apple_sysroot("iphonesimulator"),
      .app_exe_path =
          [](auto &exe, auto stem) {
            impl::apple_bundle_path(exe, stem);
            sys::path::append(exe, stem);
          },
      .app_res_path = [](auto exe) { sys::path::remove_filename(exe); },
      .bundle = [](auto &exe, auto stem) {},
  };
}

context windows() {
  static SmallVector<StringRef, 3> predefs{{
      "LECO_TARGET_WINDOWS",
  }};
  return context{
      .predefs = predefs,
      .target = "x86_64-pc-windows-msvc",
      .app_exe_path =
          [](auto exe, auto stem) {
            impl::apple_bundle_path(exe, stem);
            sys::path::append(exe, stem);
            sys::path::replace_extension(exe, "exe");
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
