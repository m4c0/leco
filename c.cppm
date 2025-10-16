module;
#include <stdlib.h>
#include <string.h>

export module c;

export namespace c {
  const auto atoi = ::atoi;

  const auto strcmp = ::strcmp;
  const auto strlen = ::strlen;
  const auto strncmp = ::strncmp;

  // These have overloads in certain platforms
  char * (*strchr)(const char *, int) = ::strchr;
}
