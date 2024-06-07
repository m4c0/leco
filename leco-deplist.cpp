#pragma leco tool
#define GOPT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "dag2.hpp"
#include "die.hpp"
#include "fopen.hpp"
#include "gopt.hpp"
#include "in2out.hpp"
#include "sim.hpp"

#include <set>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>

static FILE *out{};
static const char *target{};
static const char *argv0{};

static std::set<std::string> added{};

static void usage() {
  die(R"(
Usage: %s -i <input>

Where:
        -i: input DAG file (must be inside the "out" folder)
)",
      argv0);
}

static void print_dep(const char *dag) {
  sim_sbt stem{};
  sim_sb_path_copy_stem(&stem, dag);

  auto *c = strchr(stem.buffer, '-');
  if (c != nullptr)
    *c = ':';

  sim_sbt pcm{};
  sim_sb_copy(&pcm, dag);
  sim_sb_path_set_extension(&pcm, "pcm");

  for (auto *c = pcm.buffer; *c; c++)
    if (*c == '\\')
      *c = '/';

  fprintf(out, "-fmodule-file=%s=%s\n", stem.buffer, pcm.buffer);
}

static void read_dag(const char *dag) {
  auto [_, inserted] = added.insert(dag);
  if (!inserted)
    return;

  print_dep(dag);

  dag_read(dag, [](auto id, auto file) {
    switch (id) {
    case 'mdep': {
      sim_sbt ddag{};
      in2out(file, &ddag, "dag", target);
      read_dag(ddag.buffer);
      break;
    }
    default:
      break;
    }
  });
}

void run(int argc, char **argv) {
  argv0 = argv[0];

  const char *input{};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    default:
      usage();
    }
  });
  if (opts.argc != 0)
    usage();
  if (!*input)
    usage();
  if (!strstr(input, SIM_PATHSEP_S "out" SIM_PATHSEP_S))
    usage();

  sim_sbt output{};
  sim_sb_copy(&output, input);
  sim_sb_path_set_extension(&output, "deps");

  f::open f{output.buffer, "wb"};
  out = *f;

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, output.buffer);
  target = sim_sb_path_filename(&path);

  read_dag(input);
}

int main(int argc, char **argv) {
  try {
    run(argc, argv);
    return 0;
  } catch (...) {
    return 1;
  }
}
