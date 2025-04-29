#ifdef __APPLE__
#pragma leco tool
#pragma leco add_impl ipa_plist
#endif

#include <string.h>

import gopt;
import plist;
import sim;
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

void gen_iphone_ipa(const char * exe, const char * dag);

static void copy(const char * with, const char * dag, const char * to) {
  sys::tool_run(with, " -i %s -o %s", dag, to);
}

static void xcassets(const char * dag, const char * app_path) {
  sys::tool_run("xcassets", " -i %s -a %s", dag, app_path);
}

static void export_archive() {
  sys::tool_run("ipa-export");
}

static void upload_archive(const char * dag) {
  auto method = sys::env("LECO_IOS_METHOD");
  if (0 != strcmp("app-store-connect", method)) return;

  sys::tool_run("ipa-upload", " -i %s -s", dag);
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
  auto path = sim::sb { dag };
  path.path_parent();
  path /= "export.xcarchive";
  path /= "Products";
  path /= "Applications";
  path /= sim::path_filename(dag);
  path.path_extension("app");

  copy("exs", dag, *path);
  copy("rsrc", dag, *path);
  xcassets(dag, *path);

  path /= sim::path_filename(dag);
  path.path_extension("exe");
  gen_iphone_ipa(*path, dag);

  export_archive();
  upload_archive(dag);
}

// TODO: consider remove "input" parameter like other tools
int main(int argc, char ** argv) try {
  const char * input {};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      default: usage(); break;
    }
  });
  if (!input || opts.argc != 0) usage();

  auto path = sim::path_parent(input);
  auto target = path.path_filename();

  if (sys::is_tgt_iphoneos(target)) iphone_bundle(input);
  else if (sys::is_tgt_ios_sim(target)) iphonesim_bundle(input);
  else if (sys::is_tgt_osx(target)) osx_bundle(input);
  else usage();

  return 0;
} catch (...) {
  return 1;
}
