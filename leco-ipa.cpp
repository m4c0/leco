#ifdef __APPLE__
#pragma leco tool
#endif

#include <string.h>

import plist;
import popen;
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

// TODO: fix this for multiple exports on the same repo
static const auto out_path = "."_real / "out" / sys::target();

static void compile_launch(const char *bundle_path) {
  sys::log("ibtool", bundle_path);

  sys::runf("ibtool ../leco/launch.storyboard "
            "--compile %s/Base.lproj/launch.storyboard",
            bundle_path);
}
static void code_sign(const char *bundle_path) {
  sys::tool_run("codesign", "-d %s", bundle_path);
}
static void dump_symbols(const char * exe, const char * exca) {
  sys::log("dump symbols", exe);

  auto path = sim::sb { exca } / "dSYMs";
  sys::mkdirs(*path);
  path /= sim::path_filename(exe);
  path.path_extension(".app.dSYM");

  sys::runf("dsymutil %s -o %s", exe, *path);
}
static void gen_iphone_ipa(const char * exe, const char * dag) {
  auto app_path = sim::path_parent(exe);
  
  sim::sb exca = out_path / "export.xcarchive";

  plist::gen_info_plist(*app_path, dag, *out_path);
  compile_launch(*app_path);
  code_sign(*app_path);
  dump_symbols(exe, *exca);

  auto stem = sim::path_stem(dag);

  auto app_id = sys::read_dag_tag('apid', dag);
  if (app_id == "") app_id.printf("br.com.tpk.%s", *stem);

  auto app_ver = sys::read_dag_tag('apvr', dag);
  if (app_ver == "") app_ver = sim::sb { "1.0.0" };

  plist::gen_archive_plist(*exca, *stem, *app_id, *app_ver);
  plist::gen_export_plist(*out_path, *app_id);

  sys::log("bundle version", *plist::bundle_version);
}

static void export_archive() {
  sys::log("exporting from", *out_path);
  sys::runf("xcodebuild -exportArchive"
            " -archivePath %s/export.xcarchive"
            " -exportPath %s/export"
            " -exportOptionsPlist %s/export.plist",
            *out_path, *out_path, *out_path);
}

static void upload_archive(const char * dag) {
  auto method = sim::sb { sys::env("LECO_IOS_METHOD") };
  if (method != "app-store-connect") return;

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
      (const char *)sys::env("LECO_IOS_SIM_TARGET"),
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
