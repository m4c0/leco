#pragma leco tool

#include "sim.hpp"
#include "targets.hpp"

#include <string.h>

import gopt;
import mtime;
import plist;
import sys;

static void copy(const char *with, const char *dag, const char *to,
                 const char *extra = "") {
  sys::tool_run(with, " -i %s -o %s %s", dag, to, extra);
}

static void dir_bundle(const char *dag) {
  sim_sbt path{};
  sim_sb_copy(&path, dag);
  sim_sb_path_set_extension(&path, "app");

  copy("exs", dag, path.buffer);
  copy("rsrc", dag, path.buffer);
}

static void osx_bundle(const char *dag) {
  sim_sbt app_path {};
  sim_sb_copy(&app_path, dag);
  sim_sb_path_set_extension(&app_path, "app");

  sim_sbt cnt_path {};
  sim_sb_path_copy_append(&cnt_path, app_path.buffer, "Contents");

  sim_sbt path{};

  sim_sb_path_copy_append(&path, cnt_path.buffer, "MacOS");
  copy("exs", dag, path.buffer);

  sim_sb_path_copy_append(&path, cnt_path.buffer, "Resources");
  copy("rsrc", dag, path.buffer);

  {
    sim_sbt info{};
    sim_sb_path_copy_append(&info, path.buffer, "Info.plist");
    plist::gen(info.buffer, [&](auto &&d) {
      common_app_plist(d, "app", "macosx", "1.0.0", "0");
      d.string("CFBundleDisplayName", "app");
    });
  }

  sys::log("codesign", app_path.buffer);
  sim_sbt cmd{};
  sim_sb_printf(&cmd, "codesign -f -s %s %s", sys::env("LECO_IOS_TEAM"), app_path.buffer);
  sys::run(cmd.buffer);
}

static void iphonesim_bundle(const char * dag) {
  sim_sbt path{};
  sim_sb_copy(&path, dag);
  sim_sb_path_set_extension(&path, "app");

  copy("exs", dag, path.buffer);
  copy("rsrc", dag, path.buffer);

  sim_sbt stem {};
  sim_sb_path_copy_stem(&stem, dag);

  sim_sbt cmd {};
  sim_sb_printf(&cmd,
      "xcrun simctl install %s %s",
      sys::env("LECO_IOS_SIM_TARGET"),
      path.buffer);

  sim_sb_path_append(&path, "Info.plist");

  plist::gen(path.buffer, [&](auto &&d) {
    common_ios_plist(d, stem.buffer, stem.buffer, "0");
  });

  sys::log("installing", stem.buffer);
  sys::run(cmd.buffer);
}

static void iphone_bundle(const char *dag) {
  sys::tool_run("ipa", "-i %s", dag);
}

static void wasm_bundle(const char *dag) {
  sim_sbt path{};
  sim_sb_copy(&path, dag);
  sim_sb_path_set_extension(&path, "app");

  copy("exs", dag, path.buffer, " -e wasm");
  copy("rsrc", dag, path.buffer);

  sim_sb_path_append(&path, sim_path_filename(dag));
  sim_sb_path_set_extension(&path, "html");
  if (mtime::of("../leco/wasm.html") > mtime::of(path.buffer)) {
    sys::log("copying", path.buffer);
    sys::link("../leco/wasm.html", path.buffer);
  }

  sim_sb_path_parent(&path);

  sys::tool_run("wasm-js", " -i %s -a %s", dag, path.buffer);
}

static void bundle(const char *dag) {
  sim_sbt path{};
  sim_sb_path_copy_parent(&path, dag);
  auto target = sim_sb_path_filename(&path);

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
