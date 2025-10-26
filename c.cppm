module;
#define _CRT_NONSTDC_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

export module c;

export namespace c {
  const auto atoi = ::atoi;
  const auto remove = ::remove;
  const auto rename = ::rename;
  const auto strcmp = ::strcmp;
  const auto strdup = ::strdup;
  const auto strlen = ::strlen;
  const auto strncmp = ::strncmp;


  // This have overloads in certain platforms (and they differ between
  // platforms)
  inline auto strchr(auto str, auto c) { return ::strchr(str, c); }
}
