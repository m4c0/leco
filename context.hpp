#pragma once
#include "llvm/ADT/StringRef.h"
#include <map>
#include <set>
#include <string>

struct context {
  std::map<std::string, std::set<std::string>> pcm_dep_map{};
  std::set<std::string> pcm_reqs{};

  std::string target{};

  void add_pcm_req(llvm::StringRef path);
  void add_pcm_dep(llvm::StringRef from, llvm::StringRef to);
};

context &cur_ctx();
