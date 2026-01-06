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
  fputln(out, jojo::slurp(in_file));
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

static void gen_main_js(const char * dag, const sim::sb & path) {
  mtime::t mtime = 0;
  sys::recurse_dag(dag, [&](auto dag, auto id, auto file) {
    if (id != 'srcf') return;
    auto js = sim::sb { file }.path_extension("js");
    mtime = sys::max(mtime, mtime::of(*js));
  });

  auto output = path / "index.js";
  if (mtime::of(*output) >= mtime) return;

  sys::log("generating", *output);

  sys::file f { *output, "wb" };
  concat(f, "../leco/wasm.pre.js");

  sys::strset cache {};
  read_dag(cache, dag, f);

  auto stem = sim::path_stem(dag);
  fputln(f, jute::fmt<R"(
(function() {
  fetch("%s.wasm")
    .then(response => response.arrayBuffer())
    .then(bytes => WebAssembly.instantiate(bytes, leco_imports))
    .then(obj => {
      leco_exports = obj.instance.exports;
      obj.instance.exports._initialize();
    });
})();
)">(sv{stem}));
}

static void run(const char * dag, const char * _) {
  auto path = sim::sb { dag }.path_extension("app");

  auto html = path / "index.html";
  if (mtime::of(*html) == 0) {
    sys::log("generating", *html);

    auto name = sys::read_dag_tag('name', dag);
    if (name == "") {
      name = sim::path_stem(dag);
    }

    jojo::write(html, jute::fmt<R"(
<html>
  <head><title>%s</title></head>
  <body>
    <script type="text/javascript" src="index.js"></script>
  </body>
</html>
)">(sv { name }));
  }

  gen_main_js(dag, path);
}

int main(int argc, char **argv) try {
  if (!sys::is_tgt_wasm()) usage();
  sys::for_each_tag_in_dags('tapp', false, &run);
} catch (...) {
  return 1;
}
