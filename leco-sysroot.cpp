#pragma leco tool
#define GOPT_IMPLEMENTATION
#define PPRENT_IMPLEMENTATION
#define MTIME_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../mtime/mtime.h"
#include "../pprent/pprent.hpp"
#include "die.hpp"
#include "gopt.hpp"
#include "host_target.hpp"
#include "sim.hpp"

#include <string.h>

void usage() { die("invalid usage"); }

static bool exists(const sim_sb *path) { return mtime_of(path->buffer) > 0; }

static void find_android_llvm(sim_sb *res) {
  const auto sdk = getenv("ANDROID_SDK_ROOT");
  if (sdk == nullptr)
    die("undefined ANDROID_SDK_ROOT");

  sim_sb_path_copy_append(res, sdk, "ndk-bundle");
  if (exists(res))
    return;

  sim_sb_path_copy_append(res, sdk, "ndk");
  if (!exists(res))
    die("ANDROID_SDK_ROOT path does not contain a folder named 'sdk': [%s]",
        res->buffer);

  sim_sbt max{};
  sim_sb_copy(&max, "");

  for (auto e : pprent::list(res->buffer)) {
    if (strcmp(max.buffer, e) > 0)
      continue;

    sim_sb_copy(&max, e);
  }

  sim_sb_path_append(res, max.buffer);
  sim_sb_path_append(res, "toolchains");
  sim_sb_path_append(res, "llvm");
  sim_sb_path_append(res, "prebuilt");
  if (!exists(res))
    die("prebuilt path isn't a directory: [%s]", res->buffer);

  DIR *dir = opendir(res->buffer);
  dirent *dp = readdir(dir);
  if (dp == nullptr)
    die("no LLVM inside prebuilt dir: [%s]", res->buffer);

  sim_sb_path_append(res, dp->d_name);
  closedir(dir);
}

static const char *apple_sysroot(const char *sdk) {
#ifndef __APPLE__
  die("apple targets not supported when host isn't osx");
  return nullptr;
#else
  sim_sbt cmd{};
  sim_sb_printf(&cmd, "xcrun --show-sdk-path --sdk %s", sdk);

  static char buf[256];

  auto f = popen(cmd.buffer, "r");
  auto path = fgets(buf, sizeof(buf), f);
  pclose(f);

  if (path == nullptr)
    return nullptr;

  path[strlen(path) - 1] = 0; // chomp "\n"
  return path;
#endif
}

static const char *android_sysroot() {
  sim_sbt llvm{};
  find_android_llvm(&llvm);
  sim_sb_path_append(&llvm, "sysroot");
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

int main(int argc, char **argv) try {
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
} catch (...) {
  return 1;
}
