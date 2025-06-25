#include <stdio.h>
#include <time.h>

import plist;
import popen;
import sys;

static void compile_launch(const char *bundle_path) {
  sys::log("ibtool", bundle_path);

  sys::runf("ibtool ../leco/launch.storyboard "
            "--compile %s/Base.lproj/launch.storyboard",
            bundle_path);
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

  sys::runf("dsymutil %s -o %s", exe, *path);
}
void gen_iphone_ipa(const char * exe, const char * dag) {
  auto app_path = sim::path_parent(exe);
  
  sim::sb exca = app_path;
  exca.path_parent(); // Applications
  exca.path_parent(); // Products
  exca.path_parent(); // exports.xcarchive

  sim::sb build_path = exca;
  build_path.path_parent();

  plist::gen_info_plist(*app_path, dag, *build_path);
  compile_launch(*app_path);
  code_sign(*app_path);
  dump_symbols(exe, *exca);

  auto stem = sim::path_stem(dag);
  auto app_id = sim::printf("br.com.tpk.%s", *stem);
  sim::sb app_ver { "1.0.0" };
  sys::dag_read(dag, [&](auto id, auto val) {
    switch (id) {
      case 'apid': app_id = sim::sb { val }; break;
      case 'apvr': app_ver = sim::sb { val }; break;
      default: break;
    }
  });

  plist::gen_archive_plist(*exca, *stem, *app_id, *app_ver);
  plist::gen_export_plist(*build_path, *app_id);

  sys::log("bundle version", *plist::bundle_version);
}
