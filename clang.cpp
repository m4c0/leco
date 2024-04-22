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

  bool cpp = true;
  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'c':
      cpp = false;
      break;
    default:
      return usage();
    }
  }

  sim_sb args{};
  sim_sb_new(&args, 10240);
  if (cpp) {
    sim_sb_copy(&args, clang_cpp_exe());
    sim_sb_concat(&args, " -std=c++2b");
  } else {
    sim_sb_copy(&args, clang_c_exe());
    sim_sb_concat(&args, " -std=c11");
  }

  for (auto i = 0; i < opts.argc; i++) {
    // TODO: escape argv
    sim_sb_concat(&args, " ");
    sim_sb_concat(&args, opts.argv[i]);
  }

  return system(args.buffer);
}
