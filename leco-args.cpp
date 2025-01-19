#pragma leco tool

import gopt;
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

static str::set recursed {};
static void recurse(const char * dag) {
  if (!recursed.insert(dag)) return;

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'idag':
      case 'mdag': recurse(file); break;
      default: break;
    }
  });
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
