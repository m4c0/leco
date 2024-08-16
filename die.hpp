#pragma once
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#define _mkdir(x) mkdir((x), 0777)
#endif

struct death {};

__attribute__((format(printf, 1, 2))) inline void die(const char *msg, ...) {
  va_list arg;
  va_start(arg, msg);
  vfprintf(stderr, msg, arg);
  va_end(arg);

  fputs("\n", stderr);
  throw death{};
}
inline void run(const char *cmd) {
  if (0 != system(cmd))
    die("command failed: %s", cmd);
}

static inline void log(const char *verb, const char *msg) {
  fprintf(stderr, "%20s %s\n", verb, msg);
}
