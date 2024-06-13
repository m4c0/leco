#pragma once
#include <stdio.h>

static inline void log(const char *verb, const char *msg) {
  fprintf(stderr, "%20s %s\n", verb, msg);
}
