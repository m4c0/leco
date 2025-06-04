#pragma leco tool
#include <stdio.h>

import gopt;
import sys;

// TODO: check if files actually need regen

static void usage() {
  sys::die(R"(
Merges JS files into a single file. Only useful with WASM target. Expects JS to
have the same name as its related C++ module.

Usage: ../leco/leco.exe wasm-js -a <bundle-dir> -i <input.dag>
)");
}

static void concat(FILE *out, const char *in_file) {
  auto in = sys::fopen(in_file, "rb");

  char buf[10240];
  int got{};
  while ((got = fread(buf, 1, sizeof(buf), in)) > 0) {
    fwrite(buf, 1, got, out);
  }
}

static void run(const char * input, const char * output) {
  mtime::t mtime = 0;
  sys::recurse_dag(input, [&](auto dag, auto id, auto file) {
    if (id != 'srcf') return;
    auto js = sim::sb { file }.path_extension("js");
    mtime = sys::max(mtime, mtime::of(*js));
  });
  if (mtime::of(output) >= mtime) return;

  sys::log("generating", output);

  auto f = sys::fopen(output, "wb");
  fprintf(f, "var leco_exports;\n");
  fprintf(f, "var leco_imports = {};\n");

  sys::recurse_dag(input, [&](auto dag, auto id, auto file) {
    if (id != 'srcf') return;
    auto js = sim::sb { file }.path_extension("js");
    if (mtime::of(*js) > 0) concat(f, *js);
  });

  concat(f, "../leco/wasm.js");
}

int main(int argc, char **argv) try {
  const char * input {};
  sim::sb appdir {};

  auto opts = gopt_parse(argc, argv, "i:a:", [&](auto ch, auto val) {
    switch (ch) {
    case 'a': appdir = sim::path_real(val); break;
    case 'i': input = val; break;
    default:  usage();
    }
  });

  if (opts.argc != 0 || !input || !appdir.len) usage();

  run(input, *(appdir / "leco.js"));
} catch (...) {
  return 1;
}
