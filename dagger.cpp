#define GOPT_IMPLEMENTATION
#include "../gopt/gopt.h"

#define SIM_IMPLEMENTATION
#include "sim.h"

int usage() { return 1; }

int main(int argc, char **argv) {
  struct gopt opts;
  GOPT(opts, argc, argv, "t:");

  char *target{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 't':
      target = val;
      break;
    default:
      return usage();
    }
  }
}
