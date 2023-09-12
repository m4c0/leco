#include "droid_path.hpp"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"

using namespace llvm;

enum ndk_error {
  no_error,
  undefined_environment,
  ndk_isnt_directory,
  prebuilt_isnt_directory,
  llvm_not_found,
};

class ndk_category : public std::error_category {
public:
  const char *name() const noexcept override { return "ndk-search"; }
  std::string message(int condition) const override {
    switch (condition) {
    case no_error:
      return "No error";
    case undefined_environment:
      return "Undefined ANDROID_SDK_ROOT";
    case ndk_isnt_directory:
      return "'ndk' isn't a directory in ANDROID_SDK_ROOT";
    case prebuilt_isnt_directory:
      return "prebuilt path isn't a directory";
    case llvm_not_found:
      return "no LLVM inside prebuilt dir";
    default:
      return "unknown error";
    }
  }
};

static std::error_code find_latest_ndk(SmallVectorImpl<char> &res) {
  const auto sdk = sys::Process::GetEnv("ANDROID_SDK_ROOT");
  if (!sdk)
    return {undefined_environment, ndk_category()};

  sys::path::append(res, *sdk, "ndk-bundle");
  if (sys::fs::is_directory(res)) {
    return {};
  }

  res.clear();
  sys::path::append(res, *sdk, "ndk");
  if (!sys::fs::is_directory(res)) {
    return {ndk_isnt_directory, ndk_category()};
  }

  llvm::StringRef max = "";
  std::error_code ec;
  for (sys::fs::directory_iterator it{res, ec}, e; it != e; it.increment(ec)) {
    if (it->path() <= max)
      continue;

    max = it->path();
    res.assign(max.begin(), max.end());
  }
  return {};
}

std::error_code find_android_llvm(SmallVectorImpl<char> &res) {
  std::error_code ec = find_latest_ndk(res);
  if (ec)
    return ec;

  sys::path::append(res, "toolchains", "llvm", "prebuilt");
  if (!sys::fs::is_directory(res))
    return {prebuilt_isnt_directory, ndk_category{}};

  sys::fs::directory_iterator it{res, ec};
  if (it == sys::fs::directory_iterator{})
    return {llvm_not_found, ndk_category{}};

  res.assign(it->path().begin(), it->path().end());
  return {};
}
