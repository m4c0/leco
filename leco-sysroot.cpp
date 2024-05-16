#pragma leco tool
#define GOPT_IMPLEMENTATION

#include "die.hpp"
#include "gopt.hpp"
#include "host_target.hpp"

#include <string.h>

void usage() { die("invalid usage"); }

static const char *apple_sysroot(const char *sdk) { return nullptr; }
static const char *android_sysroot() { return nullptr; }

static const char *sysroot_for_target(const char *target) {
  if (0 == strcmp(target, "x86_64-apple-macosx11.6.0")) {
    return apple_sysroot("macosx");
  }
  if (0 == strcmp(target, "arm64-apple-ios16.1")) {
    return apple_sysroot("iphoneos");
  }
  if (0 == strcmp(target, "x86_64-apple-ios16.1-simulator")) {
    return apple_sysroot("iphonesimulator");
  }

  if (0 == strcmp(target, "aarch64-none-linux-android26")) {
    return android_sysroot();
  }
  if (0 == strcmp(target, "armv7-none-linux-androideabi26")) {
    return android_sysroot();
  }
  if (0 == strcmp(target, "i686-none-linux-android26")) {
    return android_sysroot();
  }
  if (0 == strcmp(target, "x86_64-none-linux-android26")) {
    return android_sysroot();
  }

  die("invalid target: [%s]", target);
  return nullptr;
}

int main(int argc, char **argv) {
  const char *sysroot{};
  auto opts = gopt_parse(argc, argv, "t:", [&](auto ch, auto val) {
    switch (ch) {
    case 't':
      sysroot = sysroot_for_target(val);
      break;
    default:
      usage();
      break;
    }
  });
  if (opts.argc > 0)
    usage();
}
