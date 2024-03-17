#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
const char *clang_dir() {
  static char buf[1024];
  auto f = _popen("where clang++", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  *strrchr(buf, '\\') = 0;
  *strrchr(buf, '\\') = 0;
  return buf;
}
#elif __APPLE__
const char *clang_dir() {
  static char buf[1024];
  auto f = popen("brew --prefix llvm@16", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  buf[strlen(buf) - 1] = 0;
  return buf;
}
#else
const char *clang_dir() {
  static char buf[1024];
  auto f = popen("which clang++", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  *strrchr(buf, '/') = 0;
  *strrchr(buf, '/') = 0;
  return buf;
}
#endif
