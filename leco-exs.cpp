#pragma leco tool

import sys;

static void copy_exe(const sim::sb & exedir, const char * input) {
  auto path = exedir / sim::path_filename(input);
  if (sys::is_tgt_wasm()) path.path_extension("wasm");
  sys::link(input, *path);
}

static void copy_xcfw(const sim::sb & exedir, const char * xcfw_path) {
  if (!sys::is_tgt_apple()) die("Trying to install a xcframework on a non-apple target");

  sim::sb tgt = exedir;
  if (!sys::is_tgt_ios()) tgt.path_parent();
  tgt /= "Frameworks";
  sys::mkdirs(*tgt);

  tgt /= sim::path_filename(xcfw_path);
  if (mtime::of(*tgt)) return;

  sys::runf("rsync -rav %s %s", xcfw_path, *tgt);
  sys::tool_run("codesign", "-d %s", *tgt);
}

static void copy_plugins(const sim::sb & exedir, const char * dag) {
  sys::dag_read(dag, [&](auto id, auto file) {
    if (id == 'plgn') return copy_exe(exedir, file);
  });
}

static void copy_libraries(const sim::sb & exedir, const char * dag) {
  sys::recurse_dag(dag, [&](auto dag, auto id, auto file) {
    if (id == 'dlls') return copy_exe(exedir, file);
    if (id == 'xcfw') return copy_xcfw(exedir, file);
  });
}

int main(int argc, char ** argv) try {
  sys::for_each_root_dag([](auto dag, auto id, auto file) {
    if (id != 'tapp' && id != 'tool') return;

    sim::sb edir = sys::read_dag_tag('edir', dag); 
    if (edir == "") die("dag without executable directory");

    sys::mkdirs(*edir);

    copy_exe(edir, file);
    copy_plugins(edir, dag);
    copy_libraries(edir, dag);
  });
} catch (...) {
  return 1;
}
