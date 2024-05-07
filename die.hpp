#pragma once
#include <stdarg.h>
#include <stdio.h>

struct death {};

__attribute__((format(printf, 1, 2))) inline void die(const char *msg, ...) {
  va_list arg;
  va_start(arg, msg);
  vfprintf(stderr, msg, arg);
  va_end(arg);

  fputs("", stderr);
  throw death{};
}
inline void run(const char *cmd) {
  if (0 != system(cmd))
    die("command failed: %s", cmd);
}
