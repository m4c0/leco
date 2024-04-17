#include "context.hpp"
#include "phase2.hpp"
#include "sim.hpp"

namespace t::impl {
context android(const char *tgt) {
  std::vector<std::string> predefs{};
  predefs.push_back("LECO_TARGET_ANDROID");

  sim_sbt llvm{};
  find_android_llvm(&llvm);
  sim_sb_path_append(&llvm, "sysroot");
  return context{
      .predefs = predefs,
      .target = tgt,
      .sysroot = llvm.buffer,
      .dll_ext = "so",
      .app_exe_path = [](auto exe, auto stem) {},
      .app_res_path = [](auto exe) {},
      .bundle = [](auto exe, auto stem) {},
  };
}
static std::string apple_sysroot(const char *sdk) {
#ifdef __APPLE__
  sim_sbt cmd{};
  sim_sb_printf(&cmd, "xcrun --show-sdk-path --sdk %s", sdk);

  char buf[256];

  auto f = popen(cmd.buffer, "r");
  auto path = fgets(buf, sizeof(buf), f);
  pclose(f);

  if (path == nullptr)
    return "";

  path[strlen(path) - 1] = 0; // chomp "\n"
  return path;
#else
  return "";
#endif
}
std::vector<std::string> macos_link_flags() {
  std::vector<std::string> flags{};
  flags.push_back("-rpath");
  flags.push_back("@executable_path");
  return flags;
}
std::vector<std::string> ios_link_flags() {
  std::vector<std::string> flags{};
  flags.push_back("-rpath");
  flags.push_back("@executable_path/Frameworks");
  return flags;
}
} // namespace t::impl
namespace t {
context macosx() {
  std::vector<std::string> predefs{};
  predefs.push_back("LECO_TARGET_MACOSX");
  predefs.push_back("LECO_TARGET_APPLE");
  predefs.push_back("_C99_SOURCE");
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
  std::vector<std::string> predefs{};
  predefs.push_back("LECO_TARGET_IPHONEOS");
  predefs.push_back("LECO_TARGET_IOS");
  predefs.push_back("LECO_TARGET_APPLE");
  predefs.push_back("_C99_SOURCE");
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
  std::vector<std::string> predefs{};
  predefs.push_back("LECO_TARGET_IPHONESIMULATOR");
  predefs.push_back("LECO_TARGET_IOS");
  predefs.push_back("LECO_TARGET_APPLE");
  predefs.push_back("_C99_SOURCE");
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
  std::vector<std::string> predefs{};
  predefs.push_back("LECO_TARGET_WINDOWS");
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
  std::vector<std::string> predefs{};
  predefs.push_back("LECO_TARGET_LINUX");
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

context &cur_ctx() {
  static context i{};
  return i;
}
