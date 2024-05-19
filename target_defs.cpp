#include "context.hpp"
#include "phase2.hpp"
#include "sim.hpp"

namespace t::impl {
context android(const char *tgt) {
  return context{
      .target = tgt,
      .dll_ext = "so",
      .app_exe_path = [](auto exe, auto stem) {},
      .app_res_path = [](auto exe) {},
      .bundle = [](auto exe, auto stem) {},
  };
}
std::string macos_link_flags() { return " -rpath @executable_path"; }
std::string ios_link_flags() { return " -rpath @executable_path/Frameworks"; }
} // namespace t::impl
namespace t {
context macosx() {
  return context{
      .link_flags = impl::macos_link_flags(),
      .target = "x86_64-apple-macosx11.6.0",
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
  return context{
      .link_flags = impl::ios_link_flags(),
      .target = "arm64-apple-ios16.1",
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
  return context{
      .link_flags = impl::ios_link_flags(),
      .target = "x86_64-apple-ios16.1-simulator",
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
  return context{
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
  return context{
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
