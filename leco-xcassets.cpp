#pragma leco tool

#include <stdio.h>

import gopt;
import sys;

// TODO: detect alpha in PNG - otherwise submit fails
//       with a bizarre "A 1024 x 1024 pixel app icon for your app must be
//       added to the asset catalog in Xcode." error

static void usage() {
  sys::die(R"(
Generates assets required for iOS bundling. Expects a 1024x1024 PNG icon named
`icon.png` in the source root and assumes you don't want to mess with accent
colours etc etc.

If you see werid messages from XCode about missing iOS Simulator stuff, you
need to download the iOS SDK. This command might help:

        xcodebuild -downloadPlatform iOS

Usage: ../leco/leco.exe xcassets
)");
}

static void create_xca_contents(const char *path) {
  auto file = sim::sb { path } / "Contents.json";

  fprintf(sys::file { *file, "w" }, R"({ "info": {"version": 1} })");
}

static void create_icon_contents(const char *path) {
  auto file = sim::sb { path } / "Contents.json";

  fprintf(sys::file { *file, "w" }, R"({
  "images": [{
    "filename": "Icon-1024.png",
    "idiom": "universal",
    "platform": "ios",
    "size": "1024x1024"
  }],
  "info": {"version": 1}
})");
}

static void create_colour_contents(const char *path) {
  auto file = sim::sb { path } / "Contents.json";

  fprintf(sys::file { *file, "w" }, R"({ "info": {"version": 1} })");
}

static void copy_icon(const char *path) {
  auto file = sim::sb { path } / "Icon-1024.png";
  sys::link("icon.png", *file);
}

static void run_actool(const char *plist, const char *app_path,
                       const char *xcassets) {
  sys::log("running", "actool");

  sys::runf("actool "
            "--notices --warnings --errors "
            "--output-format human-readable-text "
            "--app-icon AppIcon "
            "--accent-color AccentColor "
            "--compress-pngs "
            "--enable-on-demand-resources YES "
            "--target-device iphone "
            "--target-device ipad "
            "--platform iphoneos "
            //"--filter-for-thinning-device-configuration iPhone16,1 "
            //"--filter-for-device-os-version 17.0 "
            "--development-region en "
            "--minimum-deployment-target 17.5 "
            "--output-partial-info-plist %s "
            "--compile %s "
            "%s",
            plist, app_path, xcassets);
}

static auto gen_assets(const char * build_path) {
  auto xcassets = sim::sb { build_path } / "Assets.xcassets";

  sys::mkdirs(*xcassets);
  create_xca_contents(*xcassets);

  auto appiconset = xcassets / "AppIcon.appiconset";
  sys::mkdirs(*appiconset);
  create_icon_contents(*appiconset);
  copy_icon(*appiconset);

  auto colourset = xcassets / "AccentColor.colorset";
  sys::mkdirs(*colourset);
  create_colour_contents(*colourset);

  return xcassets;
}

static void run(const char * dag, const char * _) {
  auto app_path = sys::read_dag_tag('edir', dag);
  auto path = sim::path_parent(dag);
  auto plist = path / "icon-partial.plist";
  auto xcassets = gen_assets(*path);
  run_actool(*plist, *app_path, *xcassets);
}

int main(int argc, char **argv) try {
  if (argc != 1) usage();
  if (!sys::is_tgt_apple()) usage();

  sys::for_each_tag_in_dags('tapp', false, &run);
  return 0;
} catch (...) {
  return 1;
}
