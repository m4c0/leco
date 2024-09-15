#pragma leco tool
#include "sim.hpp"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

import gopt;
import strset;
import sys;

static FILE *out{stdout};
static const char *target{};
static const char *argv0{};

static str::set added{};

static void usage() {
  sys::die(R"(
Usage: %s -i <input> [-o <output>]

Where:
        -i: input DAG file (must be inside the "out" folder)
        -o: output file (defaults to stdout)
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
  if (!added.insert(dag))
    return;

  print_dep(dag);

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
    case 'mdag': read_dag(file); break;
    default: break;
    }
  });
}

void run(int argc, char **argv) {
  argv0 = argv[0];

  const char *input{};
  const char *output{};
  auto opts = gopt_parse(argc, argv, "i:o:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i': input = val; break;
    case 'o': output = val; break;
    default: usage();
    }
  });
  if (opts.argc != 0) usage();
  if (!*input) usage();
  if (!strstr(input, SIM_PATHSEP_S "out" SIM_PATHSEP_S)) usage();

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, input);
  target = sim_sb_path_filename(&path);

  if (output) {
    out = sys::fopen(output, "wb");
    read_dag(input);
    fclose(out);
  } else {
    read_dag(input);
  }
}

int main(int argc, char **argv) {
  try {
    run(argc, argv);
    return 0;
  } catch (...) {
    return 1;
  }
}
