module;

#include "sim.hpp"

#include <fstream>
#include <string>
#include <time.h>

export module plist;

import popen;
import sys;

export namespace plist {
constexpr const auto minimum_os_version = "17.0";
constexpr const auto platform_build = "21F77";
constexpr const auto xcode_build = "15F31d";
constexpr const auto xcode_version = "1540";

class dict {
  std::ostream &o;

  void array_element(const char *t) {
    o << "<string>";
    o << t;
    o << "</string>";
  }
  void array_element(int i) { o << "<integer>" << i << "</integer>"; }

public:
  explicit constexpr dict(std::ostream &o) : o{o} {}

  void array(const char *key, auto &&...v) {
    o << "<key>" << key << "</key><array>\n";
    (array_element(v), ...);
    o << "</array>\n";
  }
  void boolean(const char *key, bool v) {
    o << "<key>" << key << "</key>";
    o << (v ? "<true/>" : "<false/>");
    o << "\n";
  }
  void date(const char *key) {
    time_t now;
    time(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&now));
    o << "<key>" << key << "</key><date>" << buf << "</date>\n";
  }
  void dictionary(const char *key, auto &&fn) {
    o << "<key>" << key << "</key><dict>\n";
    fn(dict{o});
    o << "</dict>\n";
  }
  void integer(const char *key, int value) {
    o << "<key>" << key << "</key><integer>" << value << "</integer>\n";
  }
  void string(const char *key, const char *value) {
    o << "<key>";
    o << key;
    o << "</key><string>";
    o << value;
    o << "</string>\n";
  }
};

void gen(std::ostream &o, auto &&fn) {
  // https://developer.apple.com/library/archive/documentation/General/Reference/InfoPlistKeyReference/Introduction/Introduction.html
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

void common_app_plist(dict &d, const char *name, const char *sdk, const char * short_ver, const char * ver) {
  sim_sbt id{};
  sim_sb_printf(&id, "br.com.tpk.%s", name);
  sim_sbt exe{};
  sim_sb_printf(&exe, "%s.exe", name);

  d.string("CFBundleDevelopmentRegion", "en");
  d.string("CFBundleDisplayName", name);
  d.string("CFBundleExecutable", exe.buffer);
  d.string("CFBundleIdentifier", id.buffer);
  d.string("CFBundleInfoDictionaryVersion", "6.0");
  d.string("CFBundleName", name);
  d.string("CFBundlePackageType", "APPL");
  d.string("CFBundleShortVersionString", short_ver);
  d.string("CFBundleVersion", ver);
  d.string("DTPlatformName", sdk);
  d.string("DTPlatformBuild", platform_build);
  d.string("DTPlatformVersion", "17.5");
  d.string("DTSDKBuild", platform_build);
  d.string("DTXcodeBuild", xcode_build);
  d.string("DTXcode", xcode_version);
}
} // namespace plist
