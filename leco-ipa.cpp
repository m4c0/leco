#pragma leco tool
#pragma leco add_impl "leco-ipa_plist"

#include "sim.hpp"

import gopt;
import sys;

void usage() {
  sys::die(R"(
Exports an iOS application - i.e. generates an uploadable IPA.

If you see werid messages from XCode about missing iOS Simulator stuff, you
need to download the iOS SDK. This command might help:

        xcodebuild -downloadPlatform iOS

Usage: ../leco/leco.exe ipa -i <input.dag>

Where:
        -i: Application DAG
)");
}

static const char * tool_dir;

void gen_iphone_ipa(const char * exe_path, const char * disp_name);

static void copy(const char * with, const char * dag, const char * to) {
  sim_sbt cmd {};
  sim_sb_path_copy_append(&cmd, tool_dir, with);
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, to);
  sys::run(cmd.buffer);
}

static void xcassets(const char * dag, const char * app_path) {
  sim_sbt cmd {};
  sim_sb_path_copy_append(&cmd, tool_dir, "leco-xcassets.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  sim_sb_concat(&cmd, " -a ");
  sim_sb_concat(&cmd, app_path);
  sys::run(cmd.buffer);
}

static void export_archive(const char * build_path) {
  sim_sbt cmd { 1024 };
  sim_sb_path_copy_append(&cmd, tool_dir, "leco-ipa-export.exe");
  sys::run(cmd.buffer);
}

int main(int argc, char ** argv) try {
  const char * input {};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      default: usage(); break;
    }
  });
  if (!input || opts.argc != 0) usage();

  sim_sbt argv0 {};
  sim_sb_path_copy_parent(&argv0, argv[0]);
  tool_dir = argv0.buffer;

  sim_sbt build_path {};
  sim_sb_path_copy_parent(&build_path, input);

  sim_sbt path {};
  sim_sb_copy(&path, build_path.buffer);
  sim_sb_path_append(&path, "export.xcarchive");
  sim_sb_path_append(&path, "Products");
  sim_sb_path_append(&path, "Applications");
  sim_sb_path_append(&path, sim_path_filename(input));
  sim_sb_path_set_extension(&path, "app");
  sys::mkdirs(path.buffer);

  copy("leco-exs.exe", input, path.buffer);
  copy("leco-rsrc.exe", input, path.buffer);
  xcassets(input, path.buffer);

  sim_sbt disp_name {};
  sim_sb_path_copy_stem(&disp_name, input);
  sys::dag_read(input, [&](auto id, auto val) {
    switch (id) {
      case 'name': sim_sb_copy(&disp_name, val); break;
      default: break;
    }
  });

  sim_sb_path_append(&path, sim_path_filename(input));
  sim_sb_path_set_extension(&path, "exe");
  gen_iphone_ipa(path.buffer, disp_name.buffer);

  export_archive(build_path.buffer);

  // TODO: upload if mode is "app-store-connect"

  return 0;
} catch (...) {
  return 1;
}
