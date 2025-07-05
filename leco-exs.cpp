#pragma leco tool

#include <stdio.h>

import sys;

static void copy_exe(const sim::sb & exedir, const char * input) {
  auto path = exedir / sim::path_filename(input);
  if (sys::is_tgt_wasm()) path.path_extension("wasm");
  sys::link(input, *path);
}

static void copy_plgn(const sim::sb & exedir, const char * dll_dag) {
  auto tdll = sys::read_dag_tag('tdll', dll_dag);
  copy_exe(exedir, *tdll);
}

static void copy_xcfw(const sim::sb & exedir, const char * xcfw_path) {
  sim::sb tgt = exedir;
  if (!sys::is_tgt_ios()) tgt.path_parent();
  tgt /= "Frameworks";
  sys::mkdirs(*tgt);

  tgt /= sim::path_filename(xcfw_path);
  if (mtime::of(*tgt)) return;

  sys::runf("rsync -rav %s %s", xcfw_path, *tgt);
  sys::tool_run("codesign", "-d %s", *tgt);
}

int main(int argc, char ** argv) try {
  sys::for_each_root_dag([](auto dag, auto id, auto file) {
    if (id != 'tapp' && id != 'tool') return;

    sim::sb edir = sys::read_dag_tag('edir', dag); 
    if (edir == "") sys::die("dag without executable directory");

    sys::mkdirs(*edir);

    copy_exe(edir, file);

    sys::dag_read(dag, [&](auto id, auto file) {
      if (id == 'plgn') return copy_plgn(edir, file);
    });

    sys::recurse_dag(dag, [&](auto dag, auto id, auto file) {
      if (id == 'dlls') return copy_exe(edir, file);
      if (id == 'xcfw') return copy_xcfw(edir, file);
    });
  });
} catch (...) {
  return 1;
}
