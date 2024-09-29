#pragma leco tool

#include "sim.hpp"
#include "targets.hpp"

#include <string.h>

import gopt;
import mtime;
import plist;
import sys;

static const char *tool_dir;

static void copy(const char *with, const char *dag, const char *to,
                 const char *extra = "") {
  sim_sbt cmd{};
  sim_sb_path_copy_append(&cmd, tool_dir, with);
  sim_sb_printf(&cmd, " -i %s -o %s %s", dag, to, extra);
  sys::run(cmd.buffer);
}

static void dir_bundle(const char *dag) {
  sim_sbt path{};
  sim_sb_copy(&path, dag);
  sim_sb_path_set_extension(&path, "app");
  sys::mkdirs(path.buffer);

  copy("leco-exs.exe", dag, path.buffer);
  copy("leco-rsrc.exe", dag, path.buffer);
}

static void osx_bundle(const char *dag) {
  sim_sbt app_path {};
  sim_sb_copy(&app_path, dag);
  sim_sb_path_set_extension(&app_path, "app");

  sim_sbt path{};
  sim_sb_copy(&path, app_path.buffer);
  sim_sb_path_append(&path, "Contents");
  sys::mkdirs(path.buffer);

  {
    sim_sbt info{};
    sim_sb_path_copy_append(&info, path.buffer, "Info.plist");
    plist::gen(info.buffer, [&](auto &&d) {
      common_app_plist(d, "app", "macosx", "1.0.0", "0");
      d.string("CFBundleDisplayName", "app");
    });
  }

  sim_sb_path_append(&path, "MacOS");
  sys::mkdirs(path.buffer);
  copy("leco-exs.exe", dag, path.buffer);

  sim_sb_path_parent(&path);
  sim_sb_path_append(&path, "Resources");
  sys::mkdirs(path.buffer);
  copy("leco-rsrc.exe", dag, path.buffer);

  sys::log("codesign", app_path.buffer);
  sim_sbt cmd{};
  sim_sb_printf(&cmd, "codesign -f -s %s %s", sys::env("LECO_IOS_TEAM"), app_path.buffer);
  sys::run(cmd.buffer);
}

static void iphonesim_bundle(const char * dag) {
  sim_sbt path{};
  sim_sb_copy(&path, dag);
  sim_sb_path_set_extension(&path, "app");
  sys::mkdirs(path.buffer);

  copy("leco-exs.exe", dag, path.buffer);
  copy("leco-rsrc.exe", dag, path.buffer);

  sim_sbt stem {};
  sim_sb_path_copy_stem(&stem, dag);

  sim_sb_path_append(&path, "Info.plist");

  plist::gen(path.buffer, [&](auto &&d) {
    common_ios_plist(d, stem.buffer, stem.buffer, "0");
  });
}

static void iphone_bundle(const char *dag) {
  sim_sbt cmd{};
  sim_sb_path_copy_append(&cmd, tool_dir, "leco-ipa.exe");
  sim_sb_printf(&cmd, " -i %s", dag);
  sys::run(cmd.buffer);
}

static void wasm_bundle(const char *dag) {
  sim_sbt path{};
  sim_sb_copy(&path, dag);
  sim_sb_path_set_extension(&path, "app");
  sys::mkdirs(path.buffer);

  copy("leco-exs.exe", dag, path.buffer, " -e wasm");
  copy("leco-rsrc.exe", dag, path.buffer);

  sim_sb_path_append(&path, sim_path_filename(dag));
  sim_sb_path_set_extension(&path, "html");
  if (mtime::of("../leco/wasm.html") > mtime::of(path.buffer)) {
    sys::log("copying", path.buffer);
    sys::link("../leco/wasm.html", path.buffer);
  }

  sim_sb_path_parent(&path);

  sim_sbt cmd{};
  sim_sb_path_copy_append(&cmd, tool_dir, "leco-wasm-js.exe");
  sim_sb_printf(&cmd, " -i %s -a %s", dag, path.buffer);
  sys::run(cmd.buffer);
}

static void bundle(const char *dag) {
  sim_sbt path{};
  sim_sb_path_copy_parent(&path, dag);
  auto target = sim_sb_path_filename(&path);

  if (IS_TGT(target, TGT_IPHONEOS)) {
    iphone_bundle(dag);
  } else if (IS_TGT(target, TGT_IOS_SIMULATOR)) {
    iphonesim_bundle(dag);
  } else if (IS_TGT(target, TGT_OSX)) {
    osx_bundle(dag);
  } else if (IS_TGT(target, TGT_WASM)) {
    wasm_bundle(dag);
  } else {
    dir_bundle(dag);
  }
}

static void usage() { sys::die("invalid usage"); }

int main(int argc, char **argv) try {
  const char *input{};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    default:
      usage();
      break;
    }
  });
  if (opts.argc != 0 || !input)
    usage();

  sim_sbt argv0{};
  sim_sb_path_copy_parent(&argv0, argv[0]);
  tool_dir = argv0.buffer;

  bundle(input);
  return 0;
} catch (...) {
  return 1;
}
