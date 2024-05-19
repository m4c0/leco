#pragma once
#include <string>

struct sim_sb;

struct context {
  std::string link_flags{};
  std::string target{};
  std::string rpath{};
  std::string dll_ext{};
  void (*app_exe_path)(sim_sb *exe, const char *stem);
  void (*app_res_path)(sim_sb *exe);
  void (*bundle)(const char *exe, const char *stem);
  bool native_target{};
};

context &cur_ctx();
