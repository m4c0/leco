module;

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../mct/mct-syscall.h"

export module plist;

import sys;

export namespace plist {
constexpr const auto minimum_os_version = "17.0";
constexpr const auto platform_build = "21F77";
constexpr const auto xcode_build = "15F31d";
constexpr const auto xcode_version = "1540";

const auto bundle_version = [] {
  sim::sb buf {};
  auto t = time(nullptr);
  snprintf(*buf, buf.size - 1, "%ld", t);
  return buf;
}();

// TODO: move all "envs" here for quick ref
auto team_id() { return sys::env("LECO_IOS_TEAM"); }

class dict {
  FILE * m_f;

  void tag(const char * tag, auto value) const {
    fputln(m_f, "<", tag, ">", value, "</", tag, ">");
  }
  void tag(const char * tag) const {
    fputln(m_f, "<", tag, "/>");
  }

  void array_element(const char *t) const { tag("string",  t); }
  void array_element(int i)         const { tag("integer", i); }

public:
  explicit constexpr dict(FILE * o) : m_f { o } {}

  void array(const char *key, auto &&...v) {
    tag("key", key);
    fprintf(m_f, "<array>\n");
    (array_element(v), ...);
    fprintf(m_f, "</array>\n");
  }
  void boolean(const char *key, bool v) {
    tag("key", key);
    tag(v ? "true" : "false");
  }
  void date(const char *key) {
    time_t now;
    time(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%FT%TZ", mct_syscall_gmtime(&now));
    tag("key", key);
    tag("date", buf);
  }
  void dictionary(const char *key, auto &&fn) {
    tag("key", key);
    fprintf(m_f, "<dict>\n");
    fn(dict { m_f });
    fprintf(m_f, "</dict>\n");
  }
  void integer(const char *key, int value) {
    tag("key", key);
    tag("integer", value);
  }
  void string(const char *key, const char *value) {
    tag("key", key);
    tag("string", value);
  }

  void merge(const char * fname) {
    auto f = sys::file { fname, "rb" };

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
  gen(sys::file { fname, "wb" }, fn);
}

void common_app_plist(dict &d, const char *name, const char *sdk) {
  auto exe = sim::sb { name } + ".exe";

  d.string("CFBundleDevelopmentRegion", "en");
  d.string("CFBundleExecutable", *exe);
  d.string("CFBundleInfoDictionaryVersion", "6.0");
  d.string("CFBundleName", name);
  d.string("CFBundlePackageType", "APPL");
  d.string("CFBundleVersion", *bundle_version);
  d.string("DTPlatformName", sdk);
  d.string("DTPlatformBuild", platform_build);
  d.string("DTPlatformVersion", "17.5");
  d.string("DTSDKBuild", platform_build);
  d.string("DTXcodeBuild", xcode_build);
  d.string("DTXcode", xcode_version);
}
void common_ios_plist(dict & d, const char * dag) {
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

  common_app_plist(d, *name, "iphoneos");
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
    plist::common_ios_plist(d, dag);
  });
}
void gen_osx_plist(const char * path) {
  auto info = sim::sb { path } / "Info.plist";
  if (mtime::of(*info)) return;
  plist::gen(*info, [&](auto &&d) {
    plist::common_app_plist(d, "app", "macosx");
    d.string("CFBundleIdentifier", "br.com.tpk.app");
    d.string("CFBundleShortVersionString", "1.0.0");
    d.string("CFBundleDisplayName", "app");
  });
}

void gen_info_plist(const char * exe_path, const char * dag, const char * build_path) {
  auto path = sim::sb { exe_path } / "Info.plist";

  plist::gen(path.buffer, [&](auto &&d) {
    common_ios_plist(d, dag);

    auto plist = sim::sb { build_path } / "icon-partial.plist";
    d.merge(*plist);
  });
}
void gen_archive_plist(const char *xca_path, const char *name, const char * id, const char * app_ver) {
  auto path = sim::sb { xca_path } / "Info.plist";
  auto app_path = sim::printf("Applications/%s.app", name);

  plist::gen(path.buffer, [&](auto &&d) {
    d.dictionary("ApplicationProperties", [&](auto &&dd) {
      dd.string("ApplicationPath", app_path.buffer);
      dd.array("Architectures", "arm64");
      dd.string("CFBundleIdentifier", id);
      dd.string("CFBundleShortVersionString", app_ver);
      dd.string("CFBundleVersion", *bundle_version);
      dd.string("SigningIdentity", *sys::env("LECO_IOS_SIGN_ID"));
      dd.string("Team", *team_id());
    });
    d.integer("ArchiveVersion", 2);
    d.date("CreationDate");
    d.string("Name", name);
    d.string("SchemeName", name);
  });
}
void gen_export_plist(const char *build_path, const char * app_id) {
  auto path = sim::sb { build_path } / "export.plist";

  plist::gen(path.buffer, [&](auto &&d) {
    d.string("method", *sys::env("LECO_IOS_METHOD"));
    d.string("teamID", *team_id());
    d.string("thinning", "&lt;none&gt;");
    d.boolean("uploadSymbols", true);
    d.boolean("generateAppStoreInformation", true);
    d.dictionary("provisioningProfiles", [&](auto &&dd) {
      // TODO: detect based on installed profiles
      dd.string(app_id, *sys::env("LECO_IOS_PROV_PROF"));
    });
  });
}
} // namespace plist
