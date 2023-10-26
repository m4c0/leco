#include "actool.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

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
