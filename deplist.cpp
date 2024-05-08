#define GOPT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "die.hpp"
#include "in2out.hpp"
#include "sim.hpp"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static FILE *out{};
static const char *target{};

static void usage() { die("invalid usage"); }

static void read_dag(const char *dag);

static void print_dep(const char *file) {
  sim_sbt stem{};
  sim_sb_path_copy_stem(&stem, file);

  auto *c = strchr(stem.buffer, '-');
  if (c != nullptr)
    *c = ':';

  sim_sbt pcm{};
  in2out(file, &pcm, "pcm", target);

  sim_sbt next{};
  sim_sb_copy(&next, pcm.buffer);
  sim_sb_path_set_extension(&next, "dag");

  for (auto *c = pcm.buffer; *c; c++)
    if (*c == '\\')
      *c = '/';

  fprintf(out, "-fmodule-file=%s=%s\n", stem.buffer, pcm.buffer);

  read_dag(next.buffer);
}

static void read_dag(const char *dag) {
  FILE *f{};
  if (0 != fopen_s(&f, dag, "r"))
    die("dag file not found: [%s]\n", dag);

  char buf[10240];
  while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
    if (strlen(buf) < 5)
      die("invalid line in dag file");

    uint32_t *id = reinterpret_cast<uint32_t *>(buf);
    char *file = reinterpret_cast<char *>(id + 1);
    file[strlen(file) - 1] = 0;

    switch (*id) {
    case 'mdep':
      print_dep(file);
      break;
    default:
      break;
    }
  }

  fclose(f);
}

void run(int argc, char **argv) {
  struct gopt opts;
  GOPT(opts, argc, argv, "i:");

  const char *input{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    default:
      usage();
    }
  }
  if (opts.argc != 0)
    usage();
  if (!*input)
    usage();
  if (!strstr(input, SIM_PATHSEP_S "out" SIM_PATHSEP_S))
    usage();

  sim_sbt output{};
  sim_sb_copy(&output, input);
  sim_sb_path_set_extension(&output, "deps");

  if (0 != fopen_s(&out, output.buffer, "wb")) {
    die("could not open output file: [%s]\n", output.buffer);
  }

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, output.buffer);
  target = sim_sb_path_filename(&path);

  read_dag(input);

  fclose(out);
}

int main(int argc, char **argv) {
  try {
    run(argc, argv);
    return 0;
  } catch (...) {
    return 1;
  }
}
