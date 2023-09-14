#pragma once
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include <map>
#include <set>
#include <string>

enum class exe_t {
  none,
  tool,
  app,
};
struct dep {
  std::set<std::string> modules{};
  std::set<std::string> frameworks{};
};
struct context {
  std::map<std::string, dep> pcm_dep_map{};
  std::set<std::string> pcm_reqs{};

  llvm::ArrayRef<llvm::StringRef> predefs{};
  std::string target{};
  std::string sysroot{};
  bool native_target{};
  exe_t exe_type{};

  std::set<std::string> pending_compilation{};

  void add_pcm_req(llvm::StringRef path);
  void add_pcm_framework(llvm::StringRef path, llvm::StringRef fw);
  void add_pcm_dep(llvm::StringRef from, llvm::StringRef to);

  void add_pending(llvm::StringRef file);
};

context &cur_ctx();
