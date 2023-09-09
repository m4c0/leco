#pragma once
#include <set>
#include <string>

struct context {
  bool tool;
  std::string main_obj;
  std::set<std::string> *module_paths;
};
