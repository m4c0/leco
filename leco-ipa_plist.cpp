#include "sim.hpp"

#include <stdio.h>
#include <time.h>

import plist;
import popen;
import sys;

static const char * bundle_version;

[[nodiscard]] static const char *team_id() { return sys::env("LECO_IOS_TEAM"); }

void gen_info_plist(const char *exe_path, const char *name,
                    const char *build_path) {
  sim_sbt path{};
  sim_sb_path_copy_append(&path, exe_path, "Info.plist");

  plist::gen(path.buffer, [&](auto &&d) {
    common_app_plist(d, name, "iphoneos", "1.0.0", bundle_version);
    d.array("CFBundleSupportedPlatforms", "iPhoneOS");
    d.string("MinimumOSVersion", plist::minimum_os_version);
    d.boolean("LSRequiresIPhoneOS", true);
    d.boolean("ITSAppUsesNonExemptEncryption", false);
    d.array("UIDeviceFamily", 1); // iPhone
    d.string("UILaunchStoryboardName", "launch.storyboard");
    d.dictionary("UIRequiredDeviceCapabilities", [](auto &&dd) {
      dd.boolean("arm64", true);
      dd.boolean("metal", true);
    });
    d.array("UISupportedInterfaceOrientations",
            "UIInterfaceOrientationPortrait");

    sim_sbt plist{};
    sim_sb_path_copy_append(&plist, build_path, "icon-partial.plist");
    d.merge(plist.buffer);
  });
}
void gen_archive_plist(const char *xca_path, const char *name) {
  sim_sbt path{};
  sim_sb_path_copy_append(&path, xca_path, "Info.plist");

  sim_sbt id{};
  sim_sb_printf(&id, "br.com.tpk.%s", name);

  sim_sbt app_path{};
  sim_sb_printf(&app_path, "Applications/%s.app", name);

  plist::gen(path.buffer, [&](auto &&d) {
    d.dictionary("ApplicationProperties", [&](auto &&dd) {
      dd.string("ApplicationPath", app_path.buffer);
      dd.array("Architectures", "arm64");
      dd.string("CFBundleIdentifier", id.buffer);
      dd.string("CFBundleShortVersionString", "1.0.0");
      dd.string("CFBundleVersion", bundle_version);
      dd.string("SigningIdentity", sys::env("LECO_IOS_SIGN_ID"));
      dd.string("Team", team_id());
    });
    d.integer("ArchiveVersion", 2);
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

  plist::gen(path.buffer, [&](auto &&d) {
    d.string("method", sys::env("LECO_IOS_METHOD"));
    d.string("teamID", team_id());
    d.string("thinning", "&lt;none&gt;");
    d.boolean("uploadSymbols", false);
    d.boolean("generateAppStoreInformation", true);
    d.dictionary("provisioningProfiles", [&](auto &&dd) {
      dd.string(id.buffer, sys::env("LECO_IOS_PROV_PROF"));
    });
  });
}

static void compile_launch(const char *bundle_path) {
  sys::log("ibtool", bundle_path);

  sim_sbt cmd{};
  sim_sb_printf(&cmd,
                "ibtool ../leco/launch.storyboard "
                "--compile %s/Base.lproj/launch.storyboard",
                bundle_path);
  sys::run(cmd.buffer);
}
static void code_sign(const char *bundle_path) {
  sys::log("codesign", bundle_path);

  sim_sbt cmd{};
  sim_sb_printf(&cmd, "codesign -f -s %s %s", team_id(), bundle_path);
  sys::run(cmd.buffer);
}
void gen_iphone_ipa(const char *exe) {
  char buf[256];
  auto t = time(nullptr);
  snprintf(buf, sizeof(buf) - 1, "%ld", t);
  bundle_version = buf;

  sim_sbt name{};
  sim_sb_path_copy_stem(&name, exe);

  sim_sbt app_path{};
  sim_sb_path_copy_parent(&app_path, exe);

  sim_sbt exca{};
  sim_sb_path_copy_parent(&exca, app_path.buffer); // Applications
  sim_sb_path_parent(&exca);                       // Products
  sim_sb_path_parent(&exca);                       // exports.xcarchive

  sim_sbt build_path{};
  sim_sb_path_copy_parent(&build_path, exca.buffer);

  gen_info_plist(app_path.buffer, name.buffer, build_path.buffer);
  compile_launch(app_path.buffer);
  code_sign(app_path.buffer);

  gen_archive_plist(exca.buffer, name.buffer);
  gen_export_plist(build_path.buffer, name.buffer);
}
