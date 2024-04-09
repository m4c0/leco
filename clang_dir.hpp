#pragma once
#include "sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
static const char *clang_dir() {
  static char buf[1024];
  auto f = _popen("where clang++", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  *strrchr(buf, '\\') = 0;
  *strrchr(buf, '\\') = 0;
  return buf;
}
#elif __APPLE__
static const char *clang_dir() {
  static char buf[1024];
  auto f = popen("brew --prefix llvm@16", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  buf[strlen(buf) - 1] = 0;
  return buf;
}
#else
static const char *clang_dir() {
  static char buf[1024];
  auto f = popen("which clang++", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  *strrchr(buf, '/') = 0;
  *strrchr(buf, '/') = 0;
  return buf;
}
#endif

static const char *clang_c_exe() {
  static const auto exe = [] {
    sim_sb buf{};
    sim_sb_new(&buf, 1024);
    sim_sb_copy(&buf, clang_dir());
    sim_sb_path_append(&buf, "bin");
    sim_sb_path_append(&buf, "clang");
#ifdef _WIN32
    sim_sb_concat(&buf, ".exe");
#endif
    return buf;
  }();
  return exe.buffer;
}
static const char *clang_cpp_exe() {
  static const auto exe = [] {
    sim_sb buf{};
    sim_sb_new(&buf, 1024);
    sim_sb_copy(&buf, clang_dir());
    sim_sb_path_append(&buf, "bin");
    sim_sb_path_append(&buf, "clang++");
#ifdef _WIN32
    sim_sb_concat(&buf, ".exe");
#endif
    return buf;
  }();
  return exe.buffer;
}
static const char *clang_exe(const char *in) {
  auto e = sim_path_extension(in);
  if (0 == strcmp(e, ".c")) {
    return clang_c_exe();
  } else {
    return clang_cpp_exe();
  }
}
