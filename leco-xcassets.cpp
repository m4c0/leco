#pragma leco tool

#include "sim.hpp"

#include <stdio.h>

import gopt;
import sys;

// TODO: detect alpha in PNG - otherwise submit fails
//       with a bizarre "A 1024 x 1024 pixel app icon for your app must be
//       added to the asset catalog in Xcode." error


static void create_xca_contents(const char *path) {
  sim_sbt file{};
  sim_sb_path_copy_append(&file, path, "Contents.json");

  auto f = sys::fopen(file.buffer, "w");
  fprintf(f, R"({ "info": {"version": 1} })");
  fclose(f);
}

static void create_icon_contents(const char *path) {
  sim_sbt file{};
  sim_sb_path_copy_append(&file, path, "Contents.json");

  auto f = sys::fopen(file.buffer, "w");
  fprintf(f, R"({
  "images": [{
    "filename": "Icon-1024.png",
    "idiom": "universal",
    "platform": "ios",
    "size": "1024x1024"
  }],
  "info": {"version": 1}
})");
  fclose(f);
}

static void create_colour_contents(const char *path) {
  sim_sbt file{};
  sim_sb_path_copy_append(&file, path, "Contents.json");

  auto f = sys::fopen(file.buffer, "w");
  fprintf(f, R"({ "info": {"version": 1} })");
  fclose(f);
}

static void copy_icon(const char *path) {
  sys::log("copying", "icon.png");

  sim_sbt file{};
  sim_sb_path_copy_append(&file, path, "Icon-1024.png");
  sys::link("icon.png", file.buffer);
}

static void run_actool(const char *plist, const char *app_path,
                       const char *xcassets) {
  sys::log("running", "actool");

  sim_sbt cmd{10240};
  sim_sb_printf(&cmd,
                "actool "
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
  sys::run(cmd.buffer);
}

static void gen_assets(const char *build_path, sim_sb *xcassets) {
  sim_sb_path_copy_append(xcassets, build_path, "Assets.xcassets");

  sys::mkdirs(xcassets->buffer);
  create_xca_contents(xcassets->buffer);

  sim_sbt appiconset{};
  sim_sb_path_copy_append(&appiconset, xcassets->buffer, "AppIcon.appiconset");

  sys::mkdirs(appiconset.buffer);
  create_icon_contents(appiconset.buffer);
  copy_icon(appiconset.buffer);

  sim_sbt colourset{};
  sim_sb_path_copy_append(&colourset, xcassets->buffer, "AccentColor.colorset");

  sys::mkdirs(colourset.buffer);
  create_colour_contents(colourset.buffer);
}

static void usage() {
  sys::die(R"(
Generates assets required for iOS bundling. Expects a 1024x1024 PNG icon named
`icon.png` in the source root and assumes you don't want to mess with accent
colours etc etc.

If you see werid messages from XCode about missing iOS Simulator stuff, you
need to download the iOS SDK. This command might help:

        xcodebuild -downloadPlatform iOS

Usage: ../leco/leco.exe xcassets -i <input.dag> -a <app-path>

Where:
        -i: Application DAG
        -a: Target application folder
)");
}

int main(int argc, char **argv) try {
  const char *input{};
  const char *app_path{};
  auto opts = gopt_parse(argc, argv, "i:a:", [&](auto ch, auto val) {
    switch (ch) {
      case 'a': app_path = val; break;
      case 'i': input = val; break;
      default: usage(); break;
    }
  });
  if (!input || !app_path || opts.argc != 0) usage();

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, input);

  sim_sbt xcassets{};
  gen_assets(path.buffer, &xcassets);

  sim_sbt plist{};
  sim_sb_path_copy_append(&plist, path.buffer, "icon-partial.plist");

  run_actool(plist.buffer, app_path, xcassets.buffer);
  return 0;
} catch (...) {
  return 1;
}
