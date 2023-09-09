#pragma once
#include <set>
#include <string>

struct context {
  bool tool;
  std::set<std::string> *module_paths;
};
