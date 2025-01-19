#pragma leco tool
#include <stdio.h>

import gopt;
import mtime;
import sim;
import strset;
import sys;

static void usage() {
  sys::die(R"(
Generates a argument file containing all modules required by a C++ unit.

Usage: leco args -i <input>

Where:
        -i: input DAG file
)");
}

static void print(FILE * out, const char * file) {
  if (!out) return;
  auto stem = sim::path_stem(file);
  sim::sb pcmf { file };
  pcmf.path_extension("pcm");
  fprintf(out, "-fmodule-file=%s=%s\n", *stem, *pcmf);
}

static str::set recursed {};
static void recurse(const char * dag) {
  if (!recursed.insert(dag)) return;

  auto af = sim::sb { dag };
  af.path_extension("args");
  FILE * out {};
  if (mtime::of(dag) > mtime::of(*af)) out = sys::fopen(*af, "wb");

  sys::dag_read(dag, [out](auto id, auto file) {
    switch (id) {
      case 'idag':
      case 'mdag': print(out, file); recurse(file); break;
      case 'idir': if (out) fprintf(out, "-I%s\n", file); break;
      default: break;
    }
  });

  sys::fclose(out);
}

int main(int argc, char ** argv) try {
  const char * input {};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i': input = val; break;
    default: usage();
    }
  });
  if (opts.argc != 0) usage();
  if (!input) usage();

  recurse(input);
} catch (...) {
  return 1;
}
