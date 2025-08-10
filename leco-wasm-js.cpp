#pragma leco tool
#include <stdio.h>

import gopt;
import sys;

// TODO: check if files actually need regen

static void usage() {
  sys::die(R"(
Merges JS files into a single file. Only useful with WASM target. Expects JS to
have the same name as its related C++ module.

Usage: ../leco/leco.exe wasm-js
)");
}

static void concat(FILE *out, const char *in_file) {
  sys::file in { in_file, "rb" };

  char buf[10240];
  int got{};
  while ((got = fread(buf, 1, sizeof(buf), in)) > 0) {
    fwrite(buf, 1, got, out);
  }
}

static void run(const char * dag, const char * _) {
  auto path = sim::sb { dag }.path_extension("app");

  auto html = (path / sim::path_filename(dag)).path_extension("html");
  sys::link("../leco/wasm.html", *html);

  mtime::t mtime = 0;
  sys::recurse_dag(dag, [&](auto dag, auto id, auto file) {
    if (id != 'srcf') return;
    auto js = sim::sb { file }.path_extension("js");
    mtime = sys::max(mtime, mtime::of(*js));
  });

  auto output = path / "leco.js";
  if (mtime::of(*output) >= mtime) return;

  sys::log("generating", *output);

  sys::file f { *output, "wb" };
  concat(f, "../leco/wasm.pre.js");

  sys::recurse_dag(dag, [&](auto dag, auto id, auto file) {
    if (id != 'srcf') return;
    auto js = sim::sb { file }.path_extension("js");
    if (mtime::of(*js) > 0) concat(f, *js);
  });

  concat(f, "../leco/wasm.post.js");
}

int main(int argc, char **argv) try {
  if (!sys::is_tgt_apple()) usage();
  sys::for_each_tag_in_dags('tapp', false, &run);
} catch (...) {
  return 1;
}
