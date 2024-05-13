#pragma leco tool
#define GOPT_IMPLEMENTATION

#include "../gopt/gopt.h"

#include <stdio.h>

void usage(const char *argv0) {
  fprintf(stderr, R"(
usage: %s [-a]

where:
      -a        remove all known deps recursively

)",
          argv0);
  throw 0;
}
int main(int argc, char **argv) try {
  struct gopt opts;
  GOPT(opts, argc, argv, "a");

  bool all{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'a':
      all = true;
      break;
    default:
      usage(argv[0]);
      break;
    }
  }
  if (opts.argc != 0)
    usage(argv[0]);

  return 0;
} catch (...) {
  return 1;
}
