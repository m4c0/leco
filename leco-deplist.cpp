#pragma leco tool
#include <stdint.h>
#include <stdio.h>
#include <string.h>

import gopt;
import sim;
import strset;
import sys;

static FILE *out{stdout};
static const char *target{};
static str::set added{};

// TODO: consider merging this with leco-clang
// TODO: consider taking flag logic from leco-clang

static void usage() {
  sys::die(R"(
Generates a argument file containing all modules required by a C++ unit.

Usage: leco deplist -i <input>

Where:
        -i: input DAG file (must be inside the "out" folder)
)");
}

static void print_pcm(const char * pcmf) {
  sim::sb pcm { pcmf };
  auto stem = sim::path_stem(*pcm);

  auto * c = strchr(*stem, '-');
  if (c != nullptr) *c = ':';

  for (auto *c = *pcm; *c; c++)
    if (*c == '\\')
      *c = '/';

  fprintf(out, "-fmodule-file=%s=%s\n", *stem, *pcm);
}

static void read_dag(const char *dag) {
  if (!added.insert(dag)) return;

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
    case 'mdag': read_dag(file); break;
    case 'pcmf': print_pcm(file); break;
    default: break;
    }
  });
}
static void read_includes(const char * dag) {
  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'idir': fprintf(out, "-I%s\n", file); break;
      default: break;
    }
  });
}

int main(int argc, char **argv) try {
  const char *input{};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i': input = val; break;
    default: usage();
    }
  });
  if (opts.argc != 0) usage();
  if (!*input) usage();

  auto path = sim::path_parent(input);
  target = path.path_filename();

  sim::sb output { input };
  output.path_extension("deps");

  out = sys::fopen(*output, "wb");
  read_dag(input);
  read_includes(input);
  fclose(out);

  return 0;
} catch (...) {
  return 1;
}
