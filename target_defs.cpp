#include "context.hpp"
#include "sim.hpp"

namespace t {
context macosx() { return context{.target = "x86_64-apple-macosx11.6.0"}; }
context iphoneos() { return context{.target = "arm64-apple-ios16.1"}; }
context iphonesimulator() {
  return context{.target = "x86_64-apple-ios16.1-simulator"};
}

context windows() { return context{.target = "x86_64-pc-windows-msvc"}; }

context linux() { return context{.target = "x86_64-pc-linux-gnu"}; }

static context android(const char *tgt) { return context{.target = tgt}; }
context android_aarch64() { return android("aarch64-none-linux-android26"); }
context android_armv7() { return android("armv7-none-linux-androideabi26"); }
context android_i686() { return android("i686-none-linux-android26"); }
context android_x86_64() { return android("x86_64-none-linux-android26"); }
} // namespace t

context &cur_ctx() {
  static context i{};
  return i;
}
