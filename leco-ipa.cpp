#ifdef __APPLE__
#pragma leco tool
#pragma leco add_impl ipa_plist
#endif

#include <string.h>

import gopt;
import plist;
import sys;

void usage() {
  sys::die(R"(
Exports iOS applications - i.e. generates uploadable IPAs.

If you see werid messages from XCode about missing iOS Simulator stuff, you
need to download the iOS SDK. This command might help:

        xcodebuild -downloadPlatform iOS

Usage: ../leco/leco.exe ipa
)");
}

void gen_iphone_ipa(const char * exe, const char * dag);

static void export_archive() {
  // TODO: fix this for multiple exports on the same repo
  auto path = "."_real / "out" / sys::target();

  sys::log("exporting from", *path);
  sys::runf("xcodebuild -exportArchive"
            " -archivePath %s/export.xcarchive"
            " -exportPath %s/export"
            " -exportOptionsPlist %s/export.plist",
            *path, *path, *path);
}

static void upload_archive(const char * dag) {
  auto method = sys::env("LECO_IOS_METHOD");
  if (0 != strcmp("app-store-connect", method)) return;

  sys::tool_run("ipa-upload", " -i %s -s", dag);
}

static void osx_bundle(const char *dag) {
  auto app_path = sim::sb { dag }.path_extension("app");
  auto cnt_path = app_path / "Contents";

  plist::gen_osx_plist(*cnt_path);
  sys::tool_run("codesign", "-d %s", *app_path);
}

static void iphonesim_bundle(const char * dag) {
  auto path = sim::sb { dag }.path_extension("app");

  auto stem = sim::path_stem(dag);
  plist::gen_iphonesim_plist(*path, *stem);

  sys::log("installing", *stem);
  sys::runf(
      "xcrun simctl install %s %s",
      sys::env("LECO_IOS_SIM_TARGET"),
      *path);
}

static void iphone_bundle(const char *dag) {
  sys::tool_run("xcassets");

  auto path = sys::read_dag_tag('edir', dag) / sim::path_filename(dag);
  path.path_extension("exe");
  gen_iphone_ipa(*path, dag);

  export_archive();
  upload_archive(dag);
}

static void run(const char * dag, const char * _) {
  if (sys::is_tgt_iphoneos()) iphone_bundle(dag);
  else if (sys::is_tgt_ios_sim()) iphonesim_bundle(dag);
  else if (sys::is_tgt_osx()) osx_bundle(dag);
}

int main(int argc, char ** argv) try {
  if (argc != 1) usage();
  if (!sys::is_tgt_apple()) usage();

  sys::for_each_tag_in_dags('tapp', false, &run);
  return 0;
} catch (...) {
  return 1;
}
