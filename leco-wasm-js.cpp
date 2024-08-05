#pragma leco tool
#define SIM_IMPLEMENTATION
#include "fopen.hpp"
#include "sim.hpp"

import gopt;
import sys;

static void usage() { sys::die("invalid usage"); }

static void concat(FILE *out, const char *in_file) {
  f::open in{in_file, "rb"};

  char buf[10240];
  int got{};
  while ((got = fread(buf, 1, sizeof(buf), *in)) > 0) {
    fwrite(buf, 1, got, out);
  }
}

int main(int argc, char **argv) {
  const char *input;
  sim_sbt appdir{};

  auto opts = gopt_parse(argc, argv, "i:a:", [&](auto ch, auto val) {
    switch (ch) {
    case 'a':
      sim_sb_path_copy_real(&appdir, val);
      break;
    case 'i':
      input = val;
      break;
    default:
      usage();
    }
  });

  if (opts.argc != 0 || !input || !appdir.len)
    usage();

  sim_sb_path_append(&appdir, "leco.js");
  sys::log("generating", appdir.buffer);

  f::open f{appdir.buffer, "wb"};
  fprintf(*f, "var leco_exports;\n");
  fprintf(*f, "var leco_imports = {};\n");

  concat(*f, "../leco/wasm.js");
}
