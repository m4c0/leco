#pragma once
#include <string>

struct sim_sb;

struct context {
  std::string target{};
};

context &cur_ctx();
