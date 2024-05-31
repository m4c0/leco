#include "sim.hpp"

#include <fstream>

bool actool(const char *app_path);

namespace plist {
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

void common_app_plist(dict &d, const char *name, const char *sdk) {
  sim_sbt id{};
  sim_sb_printf(&id, "br.com.tpk.%s", name);

  d.string("CFBundleDevelopmentRegion", "en");
  d.string("CFBundleDisplayName", name);
  d.string("CFBundleExecutable", name);
  d.string("CFBundleIdentifier", id.buffer);
  d.string("CFBundleInfoDictionaryVersion", "6.0");
  d.string("CFBundlePackageType", "APPL");
  d.string("CFBundleShortVersionString", "1.0.0");
  d.string("CFBundleVersion", "1.0.0");
  d.string("DTPlatformName", sdk);
  d.string("DTPlatformBuild", "21A325");
  d.string("DTXcodeBuild", "15A240d");
}
} // namespace plist

[[nodiscard]] static const char *env(const char *key, const char *def = "TBD") {
  const auto v = getenv(key);
  return (v == nullptr) ? def : v;
}
[[nodiscard]] static const char *team_id() { return env("LECO_IOS_TEAM"); }

void merge_icon_partial(const char *build_path, std::ostream &o) {
  sim_sbt plist{};
  sim_sb_path_copy_append(&plist, build_path, "icon-partial.plist");

  auto i = std::ifstream{plist.buffer};
  std::string line;
  std::getline(i, line); // xml
  std::getline(i, line); // doctype
  std::getline(i, line); // plist
  std::getline(i, line); // dict
  while (i) {
    std::getline(i, line);
    if (line == "</dict>")
      break;
    o << line << "\n";
  }
}

void gen_info_plist(const char *exe_path, const char *name,
                    const char *build_path) {
  sim_sbt path{};
  sim_sb_path_copy_append(&path, exe_path, "Info.plist");

  std::ofstream o{path.buffer};
  plist::gen(o, [&](auto &&d) {
    common_app_plist(d, name, "iphoneos");
    d.array("CFBundleSupportedPlatforms", "iPhoneOS");
    d.string("MinimumOSVersion", "16.1");
    d.boolean("LSRequiresIPhoneOS", true);
    d.array("UIDeviceFamily", 1); // iPhone
    d.string("UILaunchStoryboardName", "launch.storyboard");
    d.dictionary("UIRequiredDeviceCapabilities", [](auto &&dd) {
      dd.boolean("arm64", true);
      dd.boolean("metal", true);
    });
    d.array("UISupportedInterfaceOrientations",
            "UIInterfaceOrientationPortrait");
    merge_icon_partial(build_path, o);
  });
}
void gen_archive_plist(const char *xca_path, const char *name) {
  sim_sbt path{};
  sim_sb_path_copy_append(&path, xca_path, "Info.plist");

  sim_sbt id{};
  sim_sb_printf(&id, "br.com.tpk.%s", name);

  sim_sbt app_path{};
  sim_sb_printf(&app_path, "Applications/%s.app", name);

  std::ofstream o{path.buffer};
  plist::gen(o, [&](auto &&d) {
    d.dictionary("ApplicationProperties", [&](auto &&dd) {
      dd.string("ApplicationPath", app_path.buffer);
      dd.array("Architectures", "arm64");
      dd.string("CFBundleIdentifier", id.buffer);
      dd.string("CFBundleShortVersionString", "1.0.0");
      dd.string("CFBundleVersion", "0");
      dd.string("SigningIdentity", env("LECO_IOS_SIGN_ID"));
      dd.string("Team", team_id());
    });
    d.integer("ArchiveVersion", 1);
    d.date("CreationDate");
    d.string("Name", name);
    d.string("SchemeName", name);
  });
}
void gen_export_plist(const char *build_path, const char *name) {
  sim_sbt path{};
  sim_sb_path_copy_append(&path, build_path, "export.plist");

  sim_sbt id{};
  sim_sb_printf(&id, "br.com.tpk.%s", name);

  std::ofstream o{path.buffer};
  plist::gen(o, [&](auto &&d) {
    d.string("method", env("LECO_IOS_METHOD", "ad-hoc"));
    d.string("teamID", team_id());
    d.string("thinning", "&lt;none&gt;");
    d.dictionary("provisioningProfiles", [&](auto &&dd) {
      dd.string(id.buffer, env("LECO_IOS_PROV_PROF"));
    });
  });
}

static bool compile_launch(const char *bundle_path) {
  sim_sbt cmd{};
  sim_sb_printf(&cmd,
                "ibtool ../leco/launch.storyboard "
                "--compile %s/Base.lproj/launch.storyboard",
                bundle_path);
  // TODO: improve error
  return 0 == std::system(cmd.buffer);
}
static bool code_sign(const char *bundle_path) {
  sim_sbt cmd{};
  sim_sb_printf(&cmd, "codesign -f -s %s %s", team_id(), bundle_path);
  // TODO: improve error
  return 0 == std::system(cmd.buffer);
}
static bool export_archive(const char *bundle_path, const char *xca_path) {
  sim_sbt cmd{1024};
  sim_sb_printf(&cmd,
                "xcodebuild -exportArchive"
                " -archivePath %s"
                " -exportPath %s/export"
                " -exportOptionsPlist %s/export.plist",
                xca_path, bundle_path, bundle_path);
  return 0 == system(cmd.buffer);
}
void gen_iphone_ipa(const char *exe) {
  sim_sbt name{};
  sim_sb_path_copy_stem(&name, exe);

  sim_sbt app_path{};
  sim_sb_path_copy_parent(&app_path, exe);

  if (!actool(app_path.buffer))
    return;

  sim_sbt exca{};
  sim_sb_path_copy_parent(&exca, app_path.buffer); // Applications
  sim_sb_path_parent(&exca);                       // Products
  sim_sb_path_parent(&exca);                       // exports.xcarchive

  sim_sbt build_path{};
  sim_sb_path_copy_parent(&build_path, exca.buffer);

  gen_info_plist(app_path.buffer, name.buffer, build_path.buffer);
  if (!compile_launch(app_path.buffer))
    return;
  if (!code_sign(app_path.buffer))
    return;

  gen_archive_plist(exca.buffer, name.buffer);
  gen_export_plist(build_path.buffer, name.buffer);
  export_archive(build_path.buffer, exca.buffer);
}
