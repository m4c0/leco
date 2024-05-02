#define GOPT_IMPLEMENTATION
#define POPEN_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "../popen/popen.h"
#include "sim.hpp"

int usage() {
  fprintf(stderr, "invalid usage\n");
  return 1;
}

static void stamp(sim_sb *args, char **&argp, const char *arg) {
  sim_sb_concat(args, " ");
  *argp++ = args->buffer + args->len;
  sim_sb_concat(args, arg);
}

int main(int argc, char **argv) {
  struct gopt opts;
  GOPT(opts, argc, argv, "t:i:");

  char *target{};
  char *source{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'i':
      source = val;
      break;
    case 't':
      target = val;
      break;
    default:
      return usage();
    }
  }

  if (source == nullptr)
    return usage();

  char *clang_argv[100]{};
  char **argp = clang_argv;

  sim_sbt args{};
  *argp++ = args.buffer;

  sim_sb_path_copy_parent(&args, argv[0]);
  sim_sb_path_append(&args, "leco-clang.exe");
  if (target != nullptr) {
    stamp(&args, argp, "-t");
    stamp(&args, argp, target);
  }
  stamp(&args, argp, "--");
  stamp(&args, argp, "-E");
  stamp(&args, argp, source);

  for (auto p = clang_argv + 1; *p && p != argp; p++) {
    (*p)[-1] = 0;
  }

  FILE *f;
  FILE *ferr;
  if (0 != proc_open(clang_argv, &f, &ferr))
    return 1;
}
}
