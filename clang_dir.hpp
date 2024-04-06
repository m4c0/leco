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

static const char *clang_exe() {
  static const auto exe = [] {
    sim_sb buf{};
    sim_sb_new(&buf, 1024);
    sim_sb_copy(&buf, clang_dir());
    sim_sb_path_append(&buf, "bin");
#ifdef _WIN32
    sim_sb_path_append(&buf, "clang++.exe");
#else
    sim_sb_path_append(&buf, "clang++");
#endif
    return buf;
  }();
  return exe.buffer;
}
