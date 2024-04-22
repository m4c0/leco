#define GOPT_IMPLEMENTATION
#include "../gopt/gopt.h"

#define SIM_IMPLEMENTATION
#include "sim.h"

static void clang_cmd(sim_sb *buf, const char *exe) {
#if __APPLE__
  auto f = popen("brew --prefix llvm@16", "r");
  if (fgets(buf->buffer, PATH_MAX, f) == nullptr)
    throw 0;

  buf->len = strlen(buf->buffer) - 1;
  sim_sb_path_append(buf, "bin");
  sim_sb_path_append(buf, exe);
#elif _WIN32
  sim_sb_path_copy(buf, exe);
  sim_sb_concat(&buf, ".exe");
#else
  sim_sb_path_copy(buf, exe);
#endif
}

int usage() {
  // TODO: print usage
  return 1;
}

int main(int argc, char **argv) {
  struct gopt opts;
  GOPT(opts, argc, argv, "cC");

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

  for (auto i = 0; i < opts.argc; i++) {
    // TODO: escape argv
    sim_sb_concat(&args, " ");
    sim_sb_concat(&args, opts.argv[i]);
  }

  return system(args.buffer);
}
