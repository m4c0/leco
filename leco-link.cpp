#pragma leco tool
#define GOPT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "die.hpp"
#include "fopen.hpp"
#include "sim.hpp"

void usage() { die("invalid usage"); }

int main(int argc, char **argv) try {
  struct gopt opts;
  GOPT(opts, argc, argv, "i:o:");

  const char *input{};
  const char *output{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    case 'o':
      output = val;
      break;
    default:
      usage();
    }
  }
  if (opts.argc != 0)
    usage();

  sim_sbt args{};
  sim_sb_copy(&args, input);
  sim_sb_path_set_extension(&args, "link");

  FILE *out{};
  if (0 != fopen_s(&out, args.buffer, "wb")) {
    die("could not open argument file: [%s]\n", args.buffer);
  }

#ifdef _WIN32 // otherwise, face LNK1107 errors from MSVC
  fputs("-fuse-ld=lld", out);
#endif
  fputs("-o", out);
  fputs(output, out);

  fclose(out);
  return 0;
} catch (...) {
  return 1;
}
