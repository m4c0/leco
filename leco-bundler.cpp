#pragma leco tool
#define SIM_IMPLEMENTATION

#include "mkdir.h"
#include "sim.hpp"
#include "targets.hpp"

import gopt;
import sys;

static const char *tool_dir;

static void copy(const char *with, const char *dag, const char *to,
                 const char *extra = "") {
  sim_sbt cmd{};
  sim_sb_path_copy_append(&cmd, tool_dir, with);
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, to);
  sim_sb_concat(&cmd, extra);
  sys::run(cmd.buffer);
}

static void dir_bundle(const char *dag) {
  sim_sbt path{};
  sim_sb_copy(&path, dag);
  sim_sb_path_set_extension(&path, "app");
  mkdirs(path.buffer);

  copy("leco-exs.exe", dag, path.buffer);
  copy("leco-rsrc.exe", dag, path.buffer);
}

static void osx_bundle(const char *dag) {
  sim_sbt path{};
  sim_sb_copy(&path, dag);
  sim_sb_path_set_extension(&path, "app");
  sim_sb_path_append(&path, "Contents");
  sim_sb_path_append(&path, "MacOS");
  mkdirs(path.buffer);
  copy("leco-exs.exe", dag, path.buffer);

  sim_sb_path_parent(&path);
  sim_sb_path_append(&path, "Resources");
  mkdirs(path.buffer);
  copy("leco-rsrc.exe", dag, path.buffer);
}

static void ios_bundle(const char *dag) {
  sim_sbt cmd{};
  sim_sb_path_copy_append(&cmd, tool_dir, "leco-ipa.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  sys::run(cmd.buffer);
}

static void wasm_bundle(const char *dag) {
  sim_sbt path{};
  sim_sb_copy(&path, dag);
  sim_sb_path_set_extension(&path, "app");
  mkdirs(path.buffer);

  copy("leco-exs.exe", dag, path.buffer, " -e wasm");
  copy("leco-rsrc.exe", dag, path.buffer);

  sim_sb_path_append(&path, sim_path_filename(dag));
  sim_sb_path_set_extension(&path, "html");
  sys::log("copying", path.buffer);
  sys::link("../leco/wasm.html", path.buffer);

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

  if (IS_TGT_IOS(target)) {
    ios_bundle(dag);
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
