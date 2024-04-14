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
static void vlog(const char *verb, const char *msg) {
  if (!is_verbose())
    return;

  if (is_tty) {
    fprintf(stderr, "\e[1;34m%20s\e[0m %s\n", verb, msg);
  } else {
    fprintf(stderr, "%20s %s\n", verb, msg);
  }
}
static void dlog(const char *verb, const char *msg) {
  fprintf(stderr, "\e[1;31m%20s\e[0m %s\n", verb, msg);
}
