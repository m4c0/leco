#pragma once
#include "cl.hpp"

#include <stdio.h>

#ifndef _WIN32
#include <unistd.h>
static const bool is_tty = isatty(fileno(stderr));
#else
#include <io.h>
static const bool is_tty = _isatty(_fileno(stderr));
#endif

// NOTE: always log to stderr to avoid order issues
static void log(unsigned colour, const char *verb, const char *msg) {
  if (is_tty) {
    fprintf(stderr, "\e[1;%dm%20s\e[0m %s\n", colour, verb, msg);
  } else {
    fprintf(stderr, "%20s %s\n", verb, msg);
  }
}
static inline void vlog(const char *verb, const char *msg) {
  if (!is_verbose())
    return;

  log(34, verb, msg);
}
static inline bool elog(const char *verb, const char *msg) {
  log(31, verb, msg);
  return false;
}
static inline void wlog(const char *verb, const char *msg) {
  log(33, verb, msg);
}
static inline void dlog(const char *verb, const char *msg) {
  log(35, verb, msg);
}
