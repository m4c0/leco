#include "droid_path.hpp"
#include "../minirent/minirent.h"
#include "../mtime/mtime.h"

static bool error(const char *msg) {
  fprintf(stderr, "error: %s\n", msg);
  return false;
}
static bool exists(const sim_sb *path) { return mtime_of(path->buffer) > 0; }

static bool find_latest_ndk(sim_sb *res) {
  const auto sdk = getenv("ANDROID_SDK_ROOT");
  if (sdk == nullptr)
    return error("undefined ANDROID_SDK_ROOT");

  sim_sb_path_copy_append(res, sdk, "ndk-bundle");
  if (exists(res))
    return true;

  sim_sb_path_copy_append(res, sdk, "ndk");
  if (!exists(res))
    return error("ANDROID_SDK_ROOT path does not contain a folder named 'sdk'");

  sim_sbt max{256};
  sim_sb_copy(&max, "");

  DIR *dir = opendir(res->buffer);
  dirent *dp = nullptr;
  while ((dp = readdir(dir)) != nullptr) {
    if (strcmp(max.buffer, dp->d_name) > 0)
      continue;

    sim_sb_copy(&max, dp->d_name);
  }
  sim_sb_path_append(res, max.buffer);

  closedir(dir);
  return true;
}

bool find_android_llvm(sim_sb *res) {
  if (!find_latest_ndk(res))
    return false;

  sim_sb_path_append(res, "toolchains");
  sim_sb_path_append(res, "llvm");
  sim_sb_path_append(res, "prebuilt");
  if (!exists(res))
    return error("prebuilt path isn't a directory");

  DIR *dir = opendir(res->buffer);
  dirent *dp = readdir(dir);
  if (dp == nullptr)
    return error("no LLVM inside prebuilt dir");

  sim_sb_path_append(res, dp->d_name);
  closedir(dir);
  return true;
}
