#pragma leco tool

#include "targets.hpp"

#include <string.h>

import gopt;
import mtime;
import plist;
import sim;
import sys;

static void copy(const char *with, const char *dag, const char *to,
                 const char *extra = "") {
  sys::tool_run(with, " -i %s -o %s %s", dag, to, extra);
}

static void dir_bundle(const char *dag) {
  sim::sb path { dag };
  path.path_extension("app");

  copy("exs", dag, *path);
  copy("rsrc", dag, *path);
}

static void osx_bundle(const char *dag) {
  sim::sb app_path { dag };
  app_path.path_extension("app");

  auto cnt_path = app_path / "Contents";

  copy("exs", dag, *(cnt_path / "MacOS"));
  copy("rsrc", dag, *(cnt_path / "Resources"));

  plist::gen_osx_plist(*cnt_path);
  sys::tool_run("codesign", "-d %s", *app_path);
}

static void iphonesim_bundle(const char * dag) {
  sim::sb path { dag };
  path.path_extension("app");

  copy("exs", dag, *path);
  copy("rsrc", dag, *path);

  auto stem = sim::path_stem(dag);
  plist::gen_iphonesim_plist(*path, *stem);

  sys::log("installing", *stem);
  sys::runf(
      "xcrun simctl install %s %s",
      sys::env("LECO_IOS_SIM_TARGET"),
      *path);
}

static void iphone_bundle(const char *dag) {
  sys::tool_run("ipa", "-i %s", dag);
}

static void wasm_bundle(const char *dag) {
  sim::sb path { dag };
  path.path_extension("app");

  copy("exs", dag, *path, " -e wasm");
  copy("rsrc", dag, *path);

  path /= sim::path_filename(dag);
  path.path_extension("html");
  if (mtime::of("../leco/wasm.html") > mtime::of(*path)) {
    sys::log("copying", *path);
    sys::link("../leco/wasm.html", *path);
  }

  path.path_parent();
  sys::tool_run("wasm-js", " -i %s -a %s", dag, *path);
}

static void bundle(const char *dag) {
  auto path = sim::path_parent(dag);
  auto target = path.path_filename();

  if (IS_TGT(target, TGT_IPHONEOS)) iphone_bundle(dag);
  else if (IS_TGT(target, TGT_IOS_SIMULATOR)) iphonesim_bundle(dag);
  else if (IS_TGT(target, TGT_OSX)) osx_bundle(dag);
  else if (IS_TGT(target, TGT_WASM)) wasm_bundle(dag);
  else dir_bundle(dag);
}

static void usage() { sys::die("invalid usage"); }

int main(int argc, char **argv) try {
  const char *input{};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      default: usage(); break;
    }
  });
  if (opts.argc != 0 || !input) usage();

  bundle(input);
  return 0;
} catch (...) {
  return 1;
}
