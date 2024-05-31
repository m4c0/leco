#pragma leco tool
#define GOPT_IMPLEMENTATION
#define MKDIR_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "die.hpp"
#include "fopen.hpp"
#include "gopt.hpp"
#include "mkdir.h"
#include "sim.hpp"

#include <filesystem>
#include <stdio.h>

namespace json {
class dict {
  FILE *f;

  void array_elements() {}
  void array_elements(auto &&v) {
    fprintf(f, "{");
    v(*this);
    fprintf(f, "}");
  }
  void array_elements(auto &&v1, auto &&v2, auto &&...vs) {
    array_elements(v1);
    fprintf(f, ",");
    array_elements(v2, vs...);
  }

public:
  explicit constexpr dict(FILE *f) : f{f} {}

  void array(const char *key, auto &&...v) {
    fprintf(f, R"("%s": [)", key);
    array_elements(v...);
    fprintf(f, "]");
  }

  void string(const char *key, const char *value, bool last = false) {
    fprintf(f, R"("%s": "%s")", key, value);
    if (!last)
      fprintf(f, ",");
  }
};
void gen(const char *path, auto &&fn) {
  sim_sbt file{};
  sim_sb_path_copy_append(&file, path, "Contents.json");

  f::open f{file.buffer, "w"};
  fprintf(*f, "{");
  fn(dict{*f});
  fprintf(*f, "}");
}
} // namespace json

static void create_xca_contents(const char *path) {
  json::gen(path, [](auto &&) {});
}

static void create_icon_contents(const char *path) {
  json::gen(path, [](auto &&d) {
    d.array("images", [](auto &&dd) {
      dd.string("filename", "icon.png");
      dd.string("idiom", "universal");
      dd.string("platform", "ios");
      dd.string("size", "1024x1024", true);
    });
  });
}

static void copy_icon(const char *path) {
  sim_sbt file{};
  sim_sb_path_copy_append(&file, path, "icon.png");
  std::filesystem::copy_file("icon.png", file.buffer);
}

static bool run_actool(const char *plist, const char *app_path,
                       const char *xcassets) {
  sim_sbt cmd{10240};
  sim_sb_printf(&cmd,
                "actool "
                "--notices --warnings --errors "
                "--output-format human-readable-text "
                "--app-icon AppIcon "
                "--accent-color AccentColor "
                "--compress-pngs "
                "--target-device iphone "
                "--target-device ipad "
                "--platform iphoneos "
                "--filter-for-thinning-device-configuration iPhone16,1 "
                "--filter-for-device-os-version 17.0 "
                "--minimum-deployment-target 17.0 "
                "--output-partial-info-plist %s "
                "--compile %s "
                "%s",
                plist, app_path, xcassets);
  return 0 == std::system(cmd.buffer);
}

static void gen_assets(const char *build_path, sim_sb *xcassets) {
  sim_sb_path_copy_append(xcassets, build_path, "Assets.xcassets");

  mkdirs(xcassets->buffer);
  create_xca_contents(xcassets->buffer);

  sim_sbt appiconset{};
  sim_sb_path_copy_append(&appiconset, xcassets->buffer, "AppIcon.appiconset");

  mkdirs(appiconset.buffer);
  create_icon_contents(appiconset.buffer);
  copy_icon(appiconset.buffer);
}

static bool actool(const char *app_path) {
  sim_sbt prod{};
  sim_sb_path_copy_parent(&prod, app_path);

  sim_sbt exca{};
  sim_sb_path_copy_parent(&exca, prod.buffer);

  sim_sbt build_path{};
  sim_sb_path_copy_parent(&build_path, exca.buffer);

  sim_sbt plist{};
  sim_sb_path_copy_append(&plist, build_path.buffer, "icon-partial.plist");

  sim_sbt xcassets{};
  gen_assets(build_path.buffer, &xcassets);

  return run_actool(plist.buffer, app_path, xcassets.buffer);
}

static void usage() { die("invalid usage"); }

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
  if (!input || opts.argc != 0)
    usage();

  return 0;
} catch (...) {
  return 1;
}
