#pragma leco tool

#include "targets.hpp"

#include <stdio.h>
#include <string.h>

import gopt;
import mtime;
import sim;
import strset;
import sys;

static const char * exedir {};
static const char * target {};
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
  if (!IS_TGT_IOS(target)) tgt.path_parent();
  tgt /= "Frameworks";

  auto cmd = sim::printf("rsync -rav %s %s", xcfw_path, *tgt);

  tgt /= sim::path_filename(xcfw_path);
  if (mtime::of(*tgt)) return;

  sys::run(*cmd);
  sys::tool_run("codesign", "-d %s", *tgt);
}

static str::set added {};
static void read_dag(const char * dag) {
  if (!added.insert(dag)) return;

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'dlls': copy_exe(file); break;
      case 'xcfw': copy_xcfw(file); break;
      case 'idag':
      case 'mdag': read_dag(file); break;
      default: break;
    }
  });
}

int main(int argc, char ** argv) try {
  const char * input {};

  auto opts = gopt_parse(argc, argv, "e:i:o:", [&](auto ch, auto val) {
    switch (ch) {
      case 'e': ext = val; break;
      case 'i': input = val; break;
      case 'o': exedir = val; break;
      default: usage(); break;
    }
  });
  if (!input || !exedir) usage();
  if (opts.argc != 0) usage();

  sys::mkdirs(exedir);

  auto path = sim::path_parent(input);
  target = path.path_filename();

  auto exe = sim::sb { input };
  exe.path_extension("exe");
  copy_exe(*exe);

  read_dag(input);
} catch (...) {
  return 1;
}
