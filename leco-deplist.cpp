#pragma leco tool
#include <stdio.h>

import gopt;
import mtime;
import sim;
import sys;

static FILE *out{stdout};

// TODO: consider merging this with leco-clang
// TODO: consider taking flag logic from leco-clang

static void print_pcm(const char * pcmf) {
  sim::sb pcm { pcmf };
  auto stem = sim::path_stem(*pcm);

  for (auto & c : stem) if (c == '-') c = ':';
  for (auto & c : pcm) if (c == '\\') c = '/';

  fprintf(out, "-fmodule-file=%s=%s\n", *stem, *pcm);
}

static void read_dag(const char *dag) {
  sys::recurse_dag(dag, [](auto dag, auto id, auto file) {
    if (id == 'pcmf') print_pcm(file);
  });
}
static void read_includes(const char * dag) {
  sys::dag_read(dag, [](auto id, auto file) {
    if (id == 'idir') fprintf(out, "-I%s\n", file);
  });
}

int main() try {
  sys::for_each_dag(sys::target(), true, [](auto dag, auto id, auto file) {
    if (id != 'vers') return;

    sim::sb output { dag };

    output.path_extension("deps");
    if (mtime::of(dag) > mtime::of(*output)) {
      out = sys::fopen(*output, "wb");
      read_dag(dag);
      fclose(out);
    }

    output.path_extension("incs");
    if (mtime::of(dag) > mtime::of(*output)) {
      out = sys::fopen(*output, "wb");
      read_includes(dag);
      fclose(out);
    }
  });

  return 0;
} catch (...) {
  return 1;
}
