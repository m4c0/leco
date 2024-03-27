#pragma once
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"

struct context {
  llvm::ArrayRef<llvm::StringRef> predefs{};
  llvm::ArrayRef<llvm::StringRef> cxx_flags{};
  llvm::ArrayRef<llvm::StringRef> link_flags{};
  std::string target{};
  std::string sysroot{};
  std::string rpath{};
  std::string dll_ext{};
  void (*app_exe_path)(llvm::SmallVectorImpl<char> &exe, llvm::StringRef stem);
  void (*app_res_path)(llvm::SmallVectorImpl<char> &exe);
  void (*bundle)(const char *exe, const char *stem);
  bool native_target{};
};

context &cur_ctx();
