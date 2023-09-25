#include "context.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace plist {
class dict {
  raw_ostream &o;

public:
  explicit constexpr dict(raw_ostream &o) : o{o} {}

  void array(StringRef key, auto &&...v) {
    o << "<key>" << key << "</key><array>\n";
    ((o << "<string>" << v << "</string>"), ...);
    o << "</array>\n";
  }
  void boolean(StringRef key, bool v) {
    o << "<key>" << key << "</key>";
    o << (v ? "<true/>" : "<false/>");
    o << "\n";
  }
  void date(StringRef key) {
    time_t now;
    time(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&now));
    o << "<key>" << key << "</key><date>" << buf << "</date>\n";
  }
  void dictionary(StringRef key, function_ref<void(dict &&)> fn) {
    o << "<key>" << key << "</key><dict>\n";
    fn(dict{o});
    o << "</dict>\n";
  }
  void integer(StringRef key, int value) {
    o << "<key>" << key << "</key><integer>" << value << "</integer>\n";
  }
  void string(Twine key, Twine value) {
    o << "<key>";
    key.print(o);
    o << "</key><string>";
    value.print(o);
    o << "</string>\n";
  }
};

void gen(raw_ostream &o, function_ref<void(dict &&)> fn) {
  o << R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
)";
  fn(plist::dict{o});
  o << R"(</dict>
</plist>
)";
}

void common_app_plist(dict &d, StringRef name, StringRef sdk) {
  d.string("CFBundleDevelopmentRegion", "en");
  d.string("CFBundleDisplayName", name);
  d.string("CFBundleExecutable", name);
  d.string("CFBundleIdentifier", "br.com.tpk." + name);
  d.string("CFBundleInfoDictionaryVersion", "6.0");
  d.string("CFBundlePackageType", "APPL");
  d.string("CFBundleShortVersionString", "1.0.0");
  d.string("CFBundleVersion", "1.0.0");
  d.string("DTPlatformName", sdk);
}
} // namespace plist

[[nodiscard]] static std::string env(const char *key) {
  const auto v = std::getenv(key);
  return (v == nullptr) ? "TBD" : std::string{v};
}

namespace plist::iphone {
void gen_info_plist(StringRef exe_path, StringRef name) {
  SmallString<256> path{};
  sys::path::append(path, exe_path, "Info.plist");
  std::error_code ec;
  auto o = raw_fd_stream(path, ec);
  gen(o, [&](auto &&d) {
    common_app_plist(d, name, "iphoneos");
    d.array("CFBundleSupportedPlatforms", "iPhoneOS");
    d.string("MinimumOSVersion", "13.0");
    d.boolean("LSRequiresIPhoneOS", true);
    d.integer("UIDeviceFamily", 1); // iPhone
  });
}
void gen_archive_plist(StringRef build_path, StringRef name) {
  SmallString<256> path{};
  sys::path::append(path, build_path, "export.xcarchive", "Info.plist");
  std::error_code ec;
  auto o = raw_fd_stream(path, ec);
  gen(o, [&](auto &&d) {
    d.dictionary("ApplicationProperties", [&](auto &&dd) {
      dd.string("ApplicationPath", "Applications/" + name + ".app");
      dd.array("Architectures", "arm64");
      dd.string("CFBundleIdentifier", "br.com.tpk." + name);
      dd.string("CFBundleShortVersionString", "1.0.0");
      dd.string("CFBundleVersion", "0");
      dd.string("SigningIdentity", env("LECO_IOS_SIGN_ID"));
      dd.string("Team", env("LECO_IOS_TEAM"));
    });
    d.integer("ArchiveVersion", 1);
    d.date("CreationDate");
    d.string("Name", name);
    d.string("SchemeName", name);
  });
}
void gen_export_plist(StringRef build_path, StringRef name) {
  SmallString<256> path{};
  sys::path::append(path, build_path, "export.plist");
  std::error_code ec;
  auto o = raw_fd_stream(path, ec);
  gen(o, [&](auto &&d) {
    d.string("method", "ad-hoc");
    d.string("teamID", env("LECO_IOS_TEAM"));
    d.string("thinning", "&lt;none&gt;");
    d.dictionary("provisioningProfiles", [&](auto &&dd) {
      dd.string("br.com.tpk." + name, env("LECO_IOS_PROV_PROF"));
    });
  });
}
} // namespace plist::iphone
