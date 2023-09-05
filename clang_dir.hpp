#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
const char *clang_dir() {
  char buf[1024];
  auto f = _popen("where clang++", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  buf[strlen(buf)] = 0;
  *strrchr(buf, '\\') = 0;
  *strrchr(buf, '\\') = 0;
  return _strdup(buf);
}
#else
const char *clang_dir() {
  char buf[1024];
  auto f = popen("brew --prefix llvm@16", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  buf[strlen(buf)] = 0;
  return _strdup(buf);
}
#endif
