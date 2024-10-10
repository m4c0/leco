#pragma leco tool
#pragma leco add_impl "leco-ipa_plist"

#include "sim.hpp"

#include <string.h>

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

void gen_iphone_ipa(const char * exe, const char * disp_name, bool landscape);

static void copy(const char * with, const char * dag, const char * to) {
  sys::tool_run(with, " -i %s -o %s", dag, to);
}

static void xcassets(const char * dag, const char * app_path) {
  sys::tool_run("xcassets", " -i %s -a %s", dag, app_path);
}

static void export_archive(const char * build_path) {
  sys::tool_run("ipa-export");
}

static void upload_archive(const char * dag) {
  auto method = sys::env("LECO_IOS_METHOD");
  if (0 != strcmp("app-store-connect", method)) return;

  sys::tool_run("ipa-upload", " -i %s -s", dag);
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

  copy("exs", input, path.buffer);
  copy("rsrc", input, path.buffer);
  xcassets(input, path.buffer);

  bool landscape {};
  sim_sbt disp_name {};
  sim_sb_path_copy_stem(&disp_name, input);
  sys::dag_read(input, [&](auto id, auto val) {
    switch (id) {
      case 'name': sim_sb_copy(&disp_name, val); break;
      case 'land': landscape = true; break;
      default: break;
    }
  });

  sim_sb_path_append(&path, sim_path_filename(input));
  sim_sb_path_set_extension(&path, "exe");
  gen_iphone_ipa(path.buffer, disp_name.buffer, landscape);

  export_archive(build_path.buffer);
  upload_archive(input);

  return 0;
} catch (...) {
  return 1;
}
