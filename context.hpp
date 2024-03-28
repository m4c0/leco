#pragma once
#include "sim.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

struct context {
  llvm::ArrayRef<llvm::StringRef> predefs{};
  llvm::ArrayRef<llvm::StringRef> cxx_flags{};
  llvm::ArrayRef<llvm::StringRef> link_flags{};
  std::string target{};
  std::string sysroot{};
  std::string rpath{};
  std::string dll_ext{};
  void (*app_exe_path)(sim_sb *exe, const char *stem);
  void (*app_res_path)(sim_sb *exe);
  void (*bundle)(const char *exe, const char *stem);
  bool native_target{};
};

context &cur_ctx();
