#pragma leco tool

#include <stdio.h>

import gopt;
import sys;

static const char * exedir {};
static const char * ext {};

static void usage() { sys::die("invalid usage"); }

static void copy_exe(const char * input) {
  auto path = sim::sb { exedir } / sim::path_filename(input);
  if (ext) path.path_extension(ext);

  if (mtime::of(*path) >= mtime::of(input)) return;

  sys::log("copying", *path);

  if (0 != remove(*path)) {
    // Rename original file. This is a "Windows-approved" way of modifying an
    // open executable.
    auto bkp = path + ".bkp";
    remove(*bkp);
    rename(*path, *bkp);
  }
  sys::link(input, *path);
}

static void copy_xcfw(const char * xcfw_path) {
  auto tgt = sim::sb { exedir };
  if (!sys::is_tgt_ios()) tgt.path_parent();
  tgt /= "Frameworks";

  auto cmd = sim::printf("rsync -rav %s %s", xcfw_path, *tgt);

  tgt /= sim::path_filename(xcfw_path);
  if (mtime::of(*tgt)) return;

  sys::run(*cmd);
  sys::tool_run("codesign", "-d %s", *tgt);
}

int main(int argc, char ** argv) try {
  const char * input {};

  auto opts = gopt_parse(argc, argv, "i:o:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      case 'o': exedir = val; break;
      default: usage(); break;
    }
  });
  if (!input || !exedir) usage();
  if (opts.argc != 0) usage();

  sys::mkdirs(exedir);

  if (sys::is_tgt_wasm()) ext = "wasm";

  auto exe = sim::sb { input }.path_extension("exe");
  copy_exe(*exe);

  sys::recurse_dag(input, [](auto dag, auto id, auto file) {
    if (id == 'dlls') return copy_exe(file);
    if (id == 'xcfw') return copy_xcfw(file);
  });
} catch (...) {
  return 1;
}
