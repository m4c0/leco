#include "actool.hpp"
#include "sim.hpp"
#include "llvm/Support/FileSystem.h"

using namespace llvm;

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
void gen(const char *path, function_ref<void(dict &&)> fn) {
  sim_sbt file{};
  sim_sb_path_copy_append(&file, path, "Contents.json");

  FILE *f = fopen(file.buffer, "w");
  fprintf(f, "{");
  fn(dict{f});
  fprintf(f, "}");
  fclose(f);
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
  sys::fs::copy_file("icon.png", file.buffer);
}

bool actool(const char *path) {
  sim_sbt app_path{};
  sim_sb_path_copy_parent(&app_path, path);

  sim_sbt prod{};
  sim_sb_path_copy_parent(&prod, app_path.buffer);

  sim_sbt exca{};
  sim_sb_path_copy_parent(&exca, prod.buffer);

  sim_sbt build_path{};
  sim_sb_path_copy_parent(&build_path, exca.buffer);

  sim_sbt plist{};
  sim_sb_path_copy_append(&plist, build_path.buffer, "icon-partial.plist");

  sim_sbt xcassets{};
  sim_sb_path_copy_append(&xcassets, build_path.buffer, "Assets.xcassets");

  sim_sbt appiconset{};
  sim_sb_path_copy_append(&appiconset, xcassets.buffer, "AppIcon.appiconset");

  sys::fs::create_directories(xcassets.buffer);
  sys::fs::create_directories(appiconset.buffer);

  create_xca_contents(xcassets.buffer);
  create_icon_contents(appiconset.buffer);
  copy_icon(appiconset.buffer);

  sim_sbt cmd{1024};
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
                plist.buffer, app_path.buffer, xcassets.buffer);
  return 0 == std::system(cmd.buffer);
}
