#pragma once
#include <string>

struct sim_sb;

struct context {
  std::string target{};
  void (*app_exe_path)(sim_sb *exe, const char *stem);
  void (*app_res_path)(sim_sb *exe);
  bool native_target{};
};

context &cur_ctx();
