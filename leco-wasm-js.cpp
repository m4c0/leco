#pragma leco tool

import jojo;
import jute;
import sys;

// TODO: check if files actually need regen

static void usage() {
  die(R"(
Merges JS files into a single file. Only useful with WASM target. Expects JS to
have the same name as its related C++ module.

Usage: ../leco/leco.exe wasm-js
)");
}

static void concat(auto & out, jute::view in_file) {
  fputln(out, jojo::read_cstr(in_file));
}

static void read_dag(sys::strset & cache, const char * dag, auto & out) {
  if (!cache.insert(dag)) return;

  sim::sb js {};
  sys::dag_read(dag, [&](auto id, auto file) {
    if (id == 'srcf') {
      js = sim::sb { file }.path_extension("js");
      if (mtime::of(*js) == 0) js = {};
      return;
    }
    if (id == 'idag' || id == 'mdag') {
      read_dag(cache, file, out);
      return;
    }
  });
  // Concat after dependencies
  if (js != "") concat(out, js);
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

  sys::strset cache {};
  read_dag(cache, dag, f);

  concat(f, "../leco/wasm.post.js");
}

int main(int argc, char **argv) try {
  if (!sys::is_tgt_wasm()) usage();
  sys::for_each_tag_in_dags('tapp', false, &run);
} catch (...) {
  return 1;
}
