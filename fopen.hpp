#pragma once
#include <stdio.h>

#ifndef _WIN32
static inline int fopen_s(FILE **fp, const char *name, const char *mode) {
  *fp = fopen(name, mode);
  return (*fp == nullptr) ? 1 : 0;
}
#endif
