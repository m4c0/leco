#pragma once
#include <set>
#include <string>

struct context {
  bool tool;
  std::string main_obj;
  std::set<std::string> object_files{};
};

context &cur_ctx() {
  static context i{};
  return i;
}
