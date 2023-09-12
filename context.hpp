#pragma once
#include "llvm/ADT/StringRef.h"
#include <map>
#include <set>
#include <string>

struct dep {
  std::set<std::string> modules{};
  std::set<std::string> ld_flags{};
};
struct context {
  std::map<std::string, dep> pcm_dep_map{};
  std::set<std::string> pcm_reqs{};

  std::string target{};

  void add_pcm_req(llvm::StringRef path);
  void add_pcm_dep(llvm::StringRef from, llvm::StringRef to);
};

context &cur_ctx();
