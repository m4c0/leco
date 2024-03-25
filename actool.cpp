#include "actool.hpp"
#include "sim.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace json {
class dict {
  raw_ostream &o;

  void array_elements() {}
  void array_elements(auto &&v) {
    o << "{";
    v(dict{o});
    o << "}";
  }
  void array_elements(auto &&v1, auto &&v2, auto &&...vs) {
    array_elements(v1);
    o << ",";
    array_elements(v2, vs...);
  }

public:
  explicit constexpr dict(raw_ostream &o) : o{o} {}

  void array(StringRef key, auto &&...v) {
    o << '"' << key << '"' << ": [";
    array_elements(v...);
    o << "]";
  }

  void string(StringRef key, StringRef value, bool last = false) {
    o << '"' << key << '"' << ": " << '"' << value << '"';
    if (!last)
      o << ",";
  }
};
void gen(const char *path, function_ref<void(dict &&)> fn) {
  sim_sbt file{256};
  sim_sb_path_append(&file, path, "Contents.json");

  std::error_code ec;
  auto o = raw_fd_stream(file.buffer, ec);

  o << "{";
  fn(dict{o});
  o << "}";
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
  sim_sbt file{256};
  sim_sb_path_append(&file, path, "icon.png");
  sys::fs::copy_file("icon.png", file.buffer);
}

bool actool(const char *path) {
  sim_sbt app_path{256};
  sim_sb_path_parent(&app_path, path);

  sim_sbt prod{256};
  sim_sb_path_parent(&prod, app_path.buffer);

  sim_sbt exca{256};
  sim_sb_path_parent(&exca, prod.buffer);

  sim_sbt build_path{256};
  sim_sb_path_parent(&build_path, exca.buffer);

  sim_sbt plist{256};
  sim_sb_path_append(&plist, build_path.buffer, "icon-partial.plist");

  sim_sbt xcassets{256};
  sim_sb_path_append(&xcassets, build_path.buffer, "Assets.xcassets");

  sim_sbt appiconset{256};
  sim_sb_path_append(&appiconset, xcassets.buffer, "AppIcon.appiconset");

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
