module;
#define _CRT_NONSTDC_NO_DEPRECATE
#include <stdlib.h>
#include <string.h>

export module c;

export namespace c {
  const auto atoi = ::atoi;

  const auto strcmp = ::strcmp;
  const auto strdup = ::strdup;
  const auto strlen = ::strlen;
  const auto strncmp = ::strncmp;

  // This have overloads in certain platforms (and they differ between
  // platforms)
  inline auto strchr(auto str, auto c) { return ::strchr(str, c); }
}
