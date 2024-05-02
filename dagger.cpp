#define GOPT_IMPLEMENTATION
#define POPEN_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "../popen/popen.h"
#include "sim.hpp"

enum class exe_t {
  none,
  dll,
  tool,
  app,
};

static char *source{};
static unsigned line{};
static exe_t exe_type{};

static int usage() {
  fprintf(stderr, "invalid usage\n");
  return 1;
}

static void error(const char *msg) {
  fprintf(stderr, "%s:%d: %s\n", source, line, msg);
  throw 0;
}

static char *cmp(char *str, const char *prefix) {
  auto len = strlen(prefix);
  if (strncmp(str, prefix, len) != 0)
    return nullptr;
  return str + len;
}

static void stamp(sim_sb *args, char **&argp, const char *arg) {
  sim_sb_concat(args, " ");
  *argp++ = args->buffer + args->len;
  sim_sb_concat(args, arg);
}

static void set_exe_type(exe_t t) {
  if (exe_type != exe_t::none)
    error("multiple executable type found");
  exe_type = t;
}

int main(int argc, char **argv) {
  struct gopt opts;
  GOPT(opts, argc, argv, "t:i:d");

  bool dump_errors{};
  char *target{};
  // FILE *out = stdout;

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'd':
      dump_errors = true;
      break;
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

  line = 0;
  sim_sbt mod_name{};

  char buf[1024];
  while (!feof(f) && fgets(buf, 1024, f) != nullptr) {
    char *p = buf;
    while (*p == ' ') {
      p++;
    }
    line++;
    if (0 == strcmp(p, "#pragma leco tool\n")) {
      set_exe_type(exe_t::tool);
    } else if (cmp(p, "#pragma leco app\n")) {
      set_exe_type(exe_t::app);
    } else if (cmp(p, "#pragma leco dll\n")) {
      set_exe_type(exe_t::dll);
    }
  }

  if (dump_errors) {
    while (!feof(ferr) && fgets(buf, 1024, ferr) != nullptr) {
      fputs(buf, stderr);
    }
  }

  switch (exe_type) {
  case exe_t::none:
    break;
  case exe_t::app:
    puts("typeapp_");
    break;
  case exe_t::dll:
    puts("typedll_");
    break;
  case exe_t::tool:
    puts("typetool");
    break;
  }

  fclose(f);
  fclose(ferr);
}
