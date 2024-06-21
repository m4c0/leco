#pragma leco tool
#define MTIME_IMPLEMENTATION
#define PPRENT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../mtime/mtime.h"
#include "../pprent/pprent.hpp"
#include "die.hpp"
#include "fopen.hpp"
#include "mkdir.h"
#include "sim.hpp"
#include "targets.hpp"

#include <string.h>

import gopt;

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
  if (IS_TGT(target, TGT_OSX)) {
    return apple_sysroot("macosx");
  } else if (IS_TGT(target, TGT_IPHONEOS)) {
    return apple_sysroot("iphoneos");
  } else if (IS_TGT(target, TGT_IOS_SIMULATOR)) {
    return apple_sysroot("iphonesimulator");
  }

  if (IS_TGT_DROID(target)) {
    return android_sysroot();
  }

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

  sim_sbt cf{};
  sim_sb_path_copy_real(&cf, argv[0]);
  sim_sb_path_parent(&cf);
  sim_sb_path_parent(&cf);
  sim_sb_path_append(&cf, target);
  mkdirs(cf.buffer);
  sim_sb_path_append(&cf, "sysroot");
  if (mtime_of(cf.buffer) > 0) {
    f::open f{cf.buffer, "r"};
    sim_sbt buf{};
    if (fgets(buf.buffer, buf.size, *f) != nullptr) {
      fwrite(buf.buffer, 1, buf.size, stdout);
      return 0;
    }
  }

  auto sysroot = sysroot_for_target(target);
  if (sysroot) {
    puts(sysroot);

    f::open f{cf.buffer, "w"};
    fputs(sysroot, *f);
  }
} catch (...) {
  return 1;
}
