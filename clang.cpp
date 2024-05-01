#define GOPT_IMPLEMENTATION
#include "../gopt/gopt.h"

#define SIM_IMPLEMENTATION
#include "sim.h"

static void clang_cmd(sim_sb *buf, const char *exe) {
#if __APPLE__
  sim_sb_copy(buf, "/usr/local/opt/llvm@16/bin");
  sim_sb_path_append(buf, exe);
#elif _WIN32
  sim_sb_copy(buf, exe);
  sim_sb_concat(buf, ".exe");
#else
  sim_sb_copy(buf, exe);
#endif
}

int usage() {
  // TODO: print usage
  return 1;
}

int main(int argc, char **argv) {
  struct gopt opts;
  GOPT(opts, argc, argv, "cCgO");

  bool debug{};
  bool opt{};
  bool cpp = true;
  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'c':
      cpp = false;
      break;
    case 'C':
      cpp = true;
      break;
    case 'g':
      debug = true;
      break;
    case 'O':
      opt = true;
      break;
    default:
      return usage();
    }
  }

  sim_sb args{};
  sim_sb_new(&args, 10240);
  if (cpp) {
    clang_cmd(&args, "clang++");
    sim_sb_concat(&args, " -std=c++2b");
  } else {
    clang_cmd(&args, "clang");
    sim_sb_concat(&args, " -std=c11");
  }
  sim_sb_concat(&args, " -Wall -Wno-unknown-pragmas");

  if (debug) {
    sim_sb_concat(&args, " -g");
  }
  if (opt) {
    sim_sb_concat(&args, " -O3 -flto -fvisibility=hidden");
  }

  for (auto i = 0; i < opts.argc; i++) {
    // TODO: escape argv
    sim_sb_concat(&args, " ");
    sim_sb_concat(&args, opts.argv[i]);
  }

  // Somehow, `system` might return 256 and our own return do a mod 256
  return 0 == system(args.buffer) ? 0 : 1;
}
