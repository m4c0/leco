module;
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

export module c;

export namespace c {
  constexpr const auto seek_set = SEEK_SET;
  constexpr const auto seek_end = SEEK_END;

  const auto atoi = ::atoi;
  const auto fclose = ::fclose;
  const auto fopen = ::fopen;
  const auto fread = ::fread;
  const auto free = ::free;
  const auto fseek = ::fseek;
  const auto ftell = ::ftell;
  const auto malloc = ::malloc;
  const auto remove = ::remove;
  const auto rename = ::rename;
  const auto strcmp = ::strcmp;
  const auto strdup = ::strdup;
  const auto strndup = ::strndup;
  const auto strlen = ::strlen;
  const auto strncmp = ::strncmp;

  // This have overloads in certain platforms (and they differ between
  // platforms)
  inline auto strchr(auto str, auto c) { return ::strchr(str, c); }
}
