#pragma leco tool
#define GOPT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "die.hpp"
#include "gopt.hpp"
#include "host_target.hpp"
#include "sim.hpp"

#include <string.h>

void usage() { die("invalid usage"); }

static const char *apple_sysroot(const char *sdk) {
#ifndef __APPLE__
  die("apple targets not supported when host isn't osx");
  return nullptr;
#else
  sim_sbt cmd{};
  sim_sb_printf(&cmd, "xcrun --show-sdk-path --sdk %s", sdk);

  char buf[256];

  auto f = popen(cmd.buffer, "r");
  auto path = fgets(buf, sizeof(buf), f);
  pclose(f);

  if (path == nullptr)
    return nullptr;

  puts(path);
  path[strlen(path) - 1] = 0; // chomp "\n"
  return path;
#endif
}

static const char *android_sysroot() {
  die("todo");
  return nullptr;
}

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
  const char *target{HOST_TARGET};
  auto opts = gopt_parse(argc, argv, "t:", [&](auto ch, auto val) {
    switch (ch) {
    case 't':
      target = val;
      break;
    default:
      usage();
      break;
    }
  });
  if (opts.argc > 0)
    usage();

  auto sysroot = sysroot_for_target(target);
  if (sysroot)
    puts(sysroot);
}
