#include "droid_path.hpp"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"

using namespace llvm;

[[nodiscard]] static inline auto find_latest_ndk(SmallVectorImpl<char> &res) {
  const auto sdk = sys::Process::GetEnv("ANDROID_SDK_ROOT");
  if (!sdk)
    return false;

  sys::path::append(res, *sdk, "ndk-bundle");
  if (sys::fs::is_directory(res)) {
    return true;
  }

  res.clear();
  sys::path::append(res, *sdk, "ndk");
  if (!sys::fs::is_directory(res)) {
    return false;
  }

  llvm::StringRef max = "";
  std::error_code ec;
  for (sys::fs::directory_iterator it{res, ec}, e; it != e; it.increment(ec)) {
    if (it->path() <= max)
      continue;

    max = it->path();
    res.assign(max.begin(), max.end());
  }
  return true;
}

std::string find_android_llvm() {
  SmallString<256> res{};
  if (!find_latest_ndk(res)) {
    throw std::runtime_error("NDK not found based on ANDROID_SDK_ROOT");
  }

  sys::path::append(res, "toolchains", "llvm", "prebuilt");
  if (!sys::fs::is_directory(res)) {
    throw std::runtime_error("LLVM not found in NDK");
  }

  std::error_code ec;
  sys::fs::directory_iterator it{res, ec};
  if (it == sys::fs::directory_iterator{})
    throw std::runtime_error("LLVM not found in NDK");

  return it->path();
}
