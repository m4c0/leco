#pragma leco tool

#include <stdio.h>

import gopt;
import sim;
import sys;

// TODO: detect alpha in PNG - otherwise submit fails
//       with a bizarre "A 1024 x 1024 pixel app icon for your app must be
//       added to the asset catalog in Xcode." error


static void create_xca_contents(const char *path) {
  auto file = sim::sb { path } / "Contents.json";

  auto f = sys::fopen(*file, "w");
  fprintf(f, R"({ "info": {"version": 1} })");
  fclose(f);
}

static void create_icon_contents(const char *path) {
  auto file = sim::sb { path } / "Contents.json";

  auto f = sys::fopen(*file, "w");
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
  auto file = sim::sb { path } / "Contents.json";

  auto f = sys::fopen(file.buffer, "w");
  fprintf(f, R"({ "info": {"version": 1} })");
  fclose(f);
}

static void copy_icon(const char *path) {
  sys::log("copying", "icon.png");

  auto file = sim::sb { path } / "Icon-1024.png";
  sys::link("icon.png", *file);
}

static void run_actool(const char *plist, const char *app_path,
                       const char *xcassets) {
  sys::log("running", "actool");

  auto cmd = sim::sb { 10240 }.printf(
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
  sys::run(*cmd);
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

  auto path = sim::sb { input };
  path.path_parent();

  auto plist = path / "icon-partial.plist";
  auto xcassets = gen_assets(*path);
  run_actool(*plist, app_path, *xcassets);
  return 0;
} catch (...) {
  return 1;
}
