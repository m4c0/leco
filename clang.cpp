#define GOPT_IMPLEMENTATION
#include "../gopt/gopt.h"

#define SIM_IMPLEMENTATION
#include "sim.h"

#if _WIN32
static const char *clang_dir() {
  static char buf[1024];
  auto f = _popen("where clang++", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  *strrchr(buf, '\\') = 0;
  *strrchr(buf, '\\') = 0;
  return buf;
}
#elif __APPLE__
static const char *clang_dir() {
  static char buf[1024];
  auto f = popen("brew --prefix llvm@16", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  buf[strlen(buf) - 1] = 0;
  return buf;
}
#else
static const char *clang_dir() {
  static char buf[1024];
  auto f = popen("which clang++", "r");
  if (fgets(buf, 1024, f) == nullptr)
    throw 0;

  *strrchr(buf, '/') = 0;
  *strrchr(buf, '/') = 0;
  return buf;
}
#endif

static const char *clang_c_exe() {
  static const auto exe = [] {
    sim_sb buf{};
    sim_sb_new(&buf, 1024);
    sim_sb_copy(&buf, clang_dir());
    sim_sb_path_append(&buf, "bin");
    sim_sb_path_append(&buf, "clang");
#ifdef _WIN32
    sim_sb_concat(&buf, ".exe");
#endif
    return buf;
  }();
  return exe.buffer;
}
static const char *clang_cpp_exe() {
  static const auto exe = [] {
    sim_sb buf{};
    sim_sb_new(&buf, 1024);
    sim_sb_copy(&buf, clang_dir());
    sim_sb_path_append(&buf, "bin");
    sim_sb_path_append(&buf, "clang++");
#ifdef _WIN32
    sim_sb_concat(&buf, ".exe");
#endif
    return buf;
  }();
  return exe.buffer;
}

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
