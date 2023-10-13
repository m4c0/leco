#pragma once
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include <map>
#include <set>
#include <string>

struct dep {
  std::set<std::string> modules{};
  std::set<std::string> frameworks{};
  std::set<std::string> resources{};
  std::set<std::string> libraries{};
  std::set<std::string> executables{};
};
struct context {
  // Defined via CLI
  llvm::ArrayRef<llvm::StringRef> predefs{};
  llvm::ArrayRef<llvm::StringRef> link_flags{};
  std::string target{};
  std::string sysroot{};
  void (*app_exe_path)(llvm::SmallVectorImpl<char> &exe, llvm::StringRef stem);
  void (*app_res_path)(llvm::SmallVectorImpl<char> &exe);
  void (*bundle)(llvm::SmallVectorImpl<char> &exe, llvm::StringRef stem);
  bool native_target{};

  // Kept per session
  std::map<std::string, dep> pcm_dep_map{};
  void add_pcm_executable(llvm::StringRef from, llvm::StringRef dll);
  void add_pcm_framework(llvm::StringRef path, llvm::StringRef fw);
  void add_pcm_dep(llvm::StringRef from, llvm::StringRef to);
  void add_pcm_resource(llvm::StringRef from, llvm::StringRef res);
  void add_pcm_library(llvm::StringRef from, llvm::StringRef lib);

  // Once per file
  std::set<std::string> pcm_reqs{};
  void add_pcm_req(llvm::StringRef path);
  void list_unique_mods(llvm::StringSet<> &out);
};

context &cur_ctx();
