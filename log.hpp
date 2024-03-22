#pragma once
#include <stdio.h>

static void vlog(const char *verb, const char *msg, int len) {
  if (is_verbose())
    fprintf(stderr, "%20s %.*s\n", verb, len, msg);
}

