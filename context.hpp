#pragma once
#include <string>
#include <vector>

struct sim_sb;

struct context {
  std::vector<std::string> predefs{};
  std::vector<std::string> cxx_flags{};
  std::vector<std::string> link_flags{};
  std::string target{};
  std::string sysroot{};
  std::string rpath{};
  std::string dll_ext{};
  void (*app_exe_path)(sim_sb *exe, const char *stem);
  void (*app_res_path)(sim_sb *exe);
  void (*bundle)(const char *exe, const char *stem);
  bool native_target{};
};

context &cur_ctx();
