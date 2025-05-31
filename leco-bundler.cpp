#pragma leco tool

import sys;

static void wasm_bundle(const char *dag) {
  auto path = sim::sb { dag }.path_extension("app");

  path /= sim::path_filename(dag);
  path.path_extension("html");
  if (mtime::of("../leco/wasm.html") > mtime::of(*path)) {
    sys::log("copying", *path);
    sys::link("../leco/wasm.html", *path);
  }

  path.path_parent();
  sys::tool_run("wasm-js", " -i %s -a %s", dag, *path);
}

static void bundle(const char * dag, const char * _) {
  if      (sys::is_tgt_apple()) sys::tool_run("ipa", "-i %s", dag);
  else if (sys::is_tgt_wasm())  wasm_bundle(dag);
}

int main() try {
  sys::for_each_tag_in_dags('tapp', false, &bundle);
  return 0;
} catch (...) {
  return 1;
}
