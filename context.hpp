#pragma once
#include <set>
#include <string>

struct context {
  std::set<std::string> object_files{};
};

context &cur_ctx();
