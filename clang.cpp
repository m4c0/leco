#define GOPT_IMPLEMENTATION
#include "../gopt/gopt.h"

#define SIM_IMPLEMENTATION
#include "clang_dir.hpp"

int usage() {
  // TODO: print usage
  return 1;
}

int main(int argc, char **argv) {
  struct gopt opts;
  GOPT(opts, argc, argv, "c");

  const char * exe = clang_cpp_exe();
  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'c':
      exe = clang_c_exe();
      break;
    default:
      return usage();
    }
  }

  if (opts.argc == 0)
    return usage();

  return system(exe);
}
