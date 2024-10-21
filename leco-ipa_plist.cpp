#include <stdio.h>
#include <time.h>

import plist;
import popen;
import sim;
import sys;

static const char * bundle_version;

[[nodiscard]] static const char *team_id() { return sys::env("LECO_IOS_TEAM"); }

static void gen_info_plist(const char *exe_path, const char *build_path, const plist::common_ios_plist_params & p) {
  auto path = sim::sb { exe_path } / "Info.plist";

  plist::gen(path.buffer, [&](auto &&d) {
    common_ios_plist(d, p);

    auto plist = sim::sb { build_path } / "icon-partial.plist";
    d.merge(*plist);
  });
}
static void gen_archive_plist(const char *xca_path, const char *name, const char * id) {
  auto path = sim::sb { xca_path } / "Info.plist";
  auto app_path = sim::printf("Applications/%s.app", name);

  plist::gen(path.buffer, [&](auto &&d) {
    d.dictionary("ApplicationProperties", [&](auto &&dd) {
      dd.string("ApplicationPath", app_path.buffer);
      dd.array("Architectures", "arm64");
      dd.string("CFBundleIdentifier", id);
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
static void gen_export_plist(const char *build_path, const char * app_id) {
  auto path = sim::sb { build_path } / "export.plist";

  plist::gen(path.buffer, [&](auto &&d) {
    d.string("method", sys::env("LECO_IOS_METHOD"));
    d.string("teamID", team_id());
    d.string("thinning", "&lt;none&gt;");
    d.boolean("uploadSymbols", true);
    d.boolean("generateAppStoreInformation", true);
    d.dictionary("provisioningProfiles", [&](auto &&dd) {
      // TODO: detect based on installed profiles
      dd.string(app_id, sys::env("LECO_IOS_PROV_PROF"));
    });
  });
}

static void compile_launch(const char *bundle_path) {
  sys::log("ibtool", bundle_path);

  auto cmd = sim::printf(
                "ibtool ../leco/launch.storyboard "
                "--compile %s/Base.lproj/launch.storyboard",
                bundle_path);
  sys::run(*cmd);
}
static void code_sign(const char *bundle_path) {
  sys::tool_run("codesign", "-d %s", bundle_path);
}
static void dump_symbols(const char * exe, const char * exca) {
  sys::log("dump symbols", exe);

  auto path = sim::sb { exca } / "dSYMs";
  sys::mkdirs(*path);
  path /= sim::path_filename(exe);
  path.path_extension(".app.dSYM");

  auto cmd = sim::printf("dsymutil %s -o %s", exe, *path);
  sys::run(*cmd);
}
void gen_iphone_ipa(const char * exe, const char * dag) {
  auto name = sim::path_stem(exe);

  bool landscape {};
  sim::sb disp_name = name;
  sim::sb app_id = sim::printf("br.com.tpk.%s", *name);
  sys::dag_read(dag, [&](auto id, auto val) {
    switch (id) {
      case 'apid': app_id = sim::sb { val }; break;
      case 'name': disp_name = sim::sb { val }; break;
      case 'land': landscape = true; break;
      default: break;
    }
  });

  char buf[256];
  auto t = time(nullptr);
  snprintf(buf, sizeof(buf) - 1, "%ld", t);
  bundle_version = buf;

  auto app_path = sim::path_parent(exe);
  
  sim::sb exca = app_path;
  exca.path_parent(); // Applications
  exca.path_parent(); // Products
  exca.path_parent(); // exports.xcarchive

  sim::sb build_path = exca;
  build_path.path_parent();

  gen_info_plist(*app_path, *build_path, {
      .name = *name, 
      .disp_name = *disp_name, 
      .bundle_version = bundle_version,
      .app_id = *app_id,
      .landscape = landscape,
  });
  compile_launch(*app_path);
  code_sign(*app_path);
  dump_symbols(exe, *exca);

  gen_archive_plist(*exca, *name, *app_id);
  gen_export_plist(*build_path, *app_id);

  sys::log("bundle version", buf);
}
