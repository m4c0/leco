#include "actool.hpp"
#include "context.hpp"
#include "droid_path.hpp"
#include "plist.hpp"
#include "llvm/ADT/SmallString.h"

using namespace llvm;

namespace t::impl {
context android(StringRef tgt) {
  static SmallVector<StringRef, 3> predefs{{
      "LECO_TARGET_ANDROID",
  }};
  sim_sbt llvm{256};
  find_android_llvm(&llvm);
  sim_sb_path_append(&llvm, "sysroot");
  return context{
      .predefs = predefs,
      .target = tgt.str(),
      .sysroot = llvm.buffer,
      .dll_ext = "so",
      .app_exe_path = [](auto exe, auto stem) {},
      .app_res_path = [](auto exe) {},
      .bundle = [](auto exe, auto stem) {},
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
      .dll_ext = "dylib",
      .app_exe_path =
          [](sim_sb *exe, const char *stem) {
            sim_sb_path_set_extension(exe, "app");
            sim_sb_path_append(exe, "Contents");
            sim_sb_path_append(exe, "MacOS");
            sim_sb_path_append(exe, stem);
            sim_sb_path_set_extension(exe, "exe");
          },
      .app_res_path =
          [](sim_sb *exe) {
            sim_sb_path_parent(exe);
            sim_sb_path_parent(exe);
            sim_sb_path_append(exe, "Resources");
          },
      .bundle = [](auto exe, auto stem) {},
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
      .dll_ext = "dylib",
      .app_exe_path =
          [](sim_sb *exe, const char *stem) {
            sim_sb_path_parent(exe);
            sim_sb_path_append(exe, "export.xcarchive");
            sim_sb_path_append(exe, "Products");
            sim_sb_path_append(exe, "Applications");
            sim_sb_path_append(exe, stem);
            sim_sb_path_set_extension(exe, "app");
            sim_sb_path_append(exe, stem);
          },
      .app_res_path = sim_sb_path_parent,
      .bundle =
          [](auto exe, auto stem) {
            if (actool(exe))
              gen_iphone_plists(exe, stem);
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
      .dll_ext = "dylib",
      .app_exe_path =
          [](sim_sb *exe, const char *stem) {
            sim_sb_path_set_extension(exe, "app");
            sim_sb_path_append(exe, stem);
          },
      .app_res_path = sim_sb_path_parent,
      .bundle = [](auto exe, auto stem) {},
  };
}

context windows() {
  static SmallVector<StringRef, 3> predefs{{
      "LECO_TARGET_WINDOWS",
  }};
  return context{
      .predefs = predefs,
      .target = "x86_64-pc-windows-msvc",
      .dll_ext = "dll",
      .app_exe_path =
          [](sim_sb *exe, const char *stem) {
            sim_sb_path_set_extension(exe, "app");
            sim_sb_path_append(exe, stem);
            sim_sb_path_set_extension(exe, "exe");
          },
      .app_res_path = sim_sb_path_parent,
      .bundle = [](auto exe, auto stem) {},
      .native_target = true,
  };
}

context linux() {
  static SmallVector<StringRef, 3> predefs{{
      "LECO_TARGET_LINUX",
  }};
  return context{
      .predefs = predefs,
      .target = "x86_64-pc-linux-gnu",
      .dll_ext = "so",
      .app_exe_path =
          [](sim_sb *exe, const char *stem) {
            sim_sb_path_set_extension(exe, "app");
            sim_sb_path_append(exe, stem);
            sim_sb_path_set_extension(exe, "exe");
          },
      .app_res_path = sim_sb_path_parent,
      .bundle = [](auto exe, auto stem) {},
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
