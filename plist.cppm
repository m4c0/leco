module;

#include <stdio.h>
#include <string.h>
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
  FILE * m_f;

  void array_element(const char *t) { fprintf(m_f, "  <string>%s</string>\n", t); }
  void array_element(int i) { fprintf(m_f, "  <integer>%d</integer>\n", i); }

public:
  explicit constexpr dict(FILE * o) : m_f { o } {}

  void array(const char *key, auto &&...v) {
    fprintf(m_f, "<key>%s</key><array>\n", key);
    (array_element(v), ...);
    fprintf(m_f, "</array>\n");
  }
  void boolean(const char *key, bool v) {
    auto val = v ? "true" : "false";
    fprintf(m_f, "<key>%s</key><%s/>\n", key, val);
  }
  void date(const char *key) {
    time_t now;
    time(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%FT%TZ", sysstd::gmtime(&now));
    fprintf(m_f, "<key>%s</key><date>%s</date>\n", key, buf);
  }
  void dictionary(const char *key, auto &&fn) {
    fprintf(m_f, "<key>%s</key><dict>\n", key);
    fn(dict { m_f });
    fprintf(m_f, "</dict>\n");
  }
  void integer(const char *key, int value) {
    fprintf(m_f, "<key>%s</key><integer>%d</integer>\n", key, value);
  }
  void string(const char *key, const char *value) {
    fprintf(m_f, "<key>%s</key><string>%s</string>\n", key, value);
  }

  void merge(const char * fname) {
    auto f = sys::file::open { fname, "rb" };

    char buf[10240];
    fgets(buf, sizeof(buf), f); // xml
    fgets(buf, sizeof(buf), f); // doctype
    fgets(buf, sizeof(buf), f); // plist
    fgets(buf, sizeof(buf), f); // dict
 
    while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
      if (0 == strcmp("</dict>", buf)) break;
      fwrite(buf, 1, strlen(buf), m_f);
    }
  }
};

void gen(FILE * f, auto && fn) {
  // https://developer.apple.com/library/archive/documentation/General/Reference/InfoPlistKeyReference/Introduction/Introduction.html
  fprintf(f, R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
)");
  fn(plist::dict { f });
  fprintf(f, "</dict>\n</plist>");
}
void gen(const char * fname, auto && fn) {
  sys::log("generating", fname);
  gen(sys::file::open { fname, "wb" }, fn);
}

void common_app_plist(dict &d, const char *name, const char *sdk, const char * ver) {
  auto exe = sim::sb { name } + ".exe";

  d.string("CFBundleDevelopmentRegion", "en");
  d.string("CFBundleExecutable", *exe);
  d.string("CFBundleInfoDictionaryVersion", "6.0");
  d.string("CFBundleName", name);
  d.string("CFBundlePackageType", "APPL");
  d.string("CFBundleVersion", ver);
  d.string("DTPlatformName", sdk);
  d.string("DTPlatformBuild", platform_build);
  d.string("DTPlatformVersion", "17.5");
  d.string("DTSDKBuild", platform_build);
  d.string("DTXcodeBuild", xcode_build);
  d.string("DTXcode", xcode_version);
}
void common_ios_plist(dict & d, const char * dag, const char * bundle_version) {
  auto name = sim::path_stem(dag);

  bool portrait {};
  bool landscape {};
  sim::sb disp_name = name;
  sim::sb app_id = sim::printf("br.com.tpk.%s", *name);
  sim::sb app_ver { "1.0.0" };
  sys::dag_read(dag, [&](auto id, auto val) {
    switch (id) {
      case 'apid': app_id = sim::sb { val }; break;
      case 'apvr': app_ver = sim::sb { val }; break;
      case 'name': disp_name = sim::sb { val }; break;
      case 'port': portrait = true; break;
      case 'land': landscape = true; break;
      default: break;
    }
  });

  common_app_plist(d, *name, "iphoneos", bundle_version);
  d.string("CFBundleIdentifier", *app_id);
  d.string("CFBundleDisplayName", *disp_name);
  d.string("CFBundleShortVersionString", *app_ver);
  d.array("CFBundleSupportedPlatforms", "iPhoneOS");
  d.string("MinimumOSVersion", plist::minimum_os_version);
  d.boolean("LSRequiresIPhoneOS", true);
  d.boolean("ITSAppUsesNonExemptEncryption", false);
  d.boolean("UIRequiresFullScreen", true);
  d.array("UIDeviceFamily", 1, 2); // iPhone
  d.string("UILaunchStoryboardName", "launch.storyboard");
  d.dictionary("UIRequiredDeviceCapabilities", [](auto &&dd) {
    dd.boolean("arm64", true);
    dd.boolean("metal", true);
  });
  if (portrait && landscape) {
    d.array("UISupportedInterfaceOrientations",
        "UIInterfaceOrientationPortrait",
        "UIInterfaceOrientationLandscapeLeft",
        "UIInterfaceOrientationLandscapeRight");
    d.array("UISupportedInterfaceOrientations~ipad",
        "UIInterfaceOrientationPortrait",
        "UIInterfaceOrientationLandscapeLeft"
        "UIInterfaceOrientationLandscapeRight");
  } else if (landscape) {
    d.array("UISupportedInterfaceOrientations",
        "UIInterfaceOrientationLandscapeLeft",
        "UIInterfaceOrientationLandscapeRight");
    d.array("UISupportedInterfaceOrientations~ipad",
        "UIInterfaceOrientationLandscapeLeft"
        "UIInterfaceOrientationLandscapeRight");
  } else {
    d.array("UISupportedInterfaceOrientations", "UIInterfaceOrientationPortrait");
    d.array("UISupportedInterfaceOrientations~ipad", "UIInterfaceOrientationPortrait");
  }
}

void gen_iphonesim_plist(const char * path, const char * dag) {
  auto info = sim::sb { path } / "Info.plist";
  if (mtime::of(*info)) return;
  plist::gen(*info, [&](auto &&d) {
    plist::common_ios_plist(d, dag, "0");
  });
}
void gen_osx_plist(const char * path) {
  auto info = sim::sb { path } / "Info.plist";
  if (mtime::of(*info)) return;
  plist::gen(*info, [&](auto &&d) {
    plist::common_app_plist(d, "app", "macosx", "0");
    d.string("CFBundleIdentifier", "br.com.tpk.app");
    d.string("CFBundleShortVersionString", "1.0.0");
    d.string("CFBundleDisplayName", "app");
  });
}
} // namespace plist
