#include "actool.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
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
void gen(StringRef path, function_ref<void(dict &&)> fn) {
  SmallString<256> file{};
  sys::path::append(file, path, "Contents.json");

  std::error_code ec;
  auto o = raw_fd_stream(file, ec);

  o << "{";
  fn(dict{o});
  o << "}";
}
} // namespace json

static void create_xca_contents(StringRef xca) {
  json::gen(xca, [](auto &&) {});
}

static void create_icon_contents(StringRef xca) {
  json::gen(xca, [](auto &&d) {
    d.array("images", [](auto &&dd) {
      dd.string("filename", "icon.png");
      dd.string("idiom", "universal");
      dd.string("platform", "ios");
      dd.string("size", "1024x1024", true);
    });
  });
}

bool actool(StringRef path) {
  auto app_path = sys::path::parent_path(path);
  auto apps = sys::path::parent_path(app_path);
  auto prod = sys::path::parent_path(apps);
  auto exca = sys::path::parent_path(prod);
  auto build_path = sys::path::parent_path(exca);

  SmallString<256> plist{};
  sys::path::append(plist, build_path, "icon-partial.plist");
  SmallString<256> xcassets{};
  sys::path::append(xcassets, build_path, "Assets.xcassets");
  SmallString<256> appiconset{};
  sys::path::append(appiconset, xcassets, "AppIcon.appiconset");

  sys::fs::create_directories(xcassets);
  sys::fs::create_directories(appiconset);

  create_xca_contents(xcassets);
  create_icon_contents(appiconset);

  auto cmd = ("actool "
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
              "--output-partial-info-plist " +
              plist + " --compile " + app_path + " " + xcassets)
                 .str();
  return 0 == std::system(cmd.c_str());
}
