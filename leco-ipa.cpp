#pragma leco tool
#pragma leco add_impl "leco-ipa_plist"

#include <string.h>

import gopt;
import sim;
import sys;

void usage() {
  sys::die(R"(
Exports an iOS application - i.e. generates an uploadable IPA.

If you see werid messages from XCode about missing iOS Simulator stuff, you
need to download the iOS SDK. This command might help:

        xcodebuild -downloadPlatform iOS

Usage: ../leco/leco.exe ipa -i <input.dag>

Where:
        -i: Application DAG
)");
}

void gen_iphone_ipa(const char * exe, const char * disp_name, bool landscape);

static void copy(const char * with, const char * dag, const char * to) {
  sys::tool_run(with, " -i %s -o %s", dag, to);
}

static void xcassets(const char * dag, const char * app_path) {
  sys::tool_run("xcassets", " -i %s -a %s", dag, app_path);
}

static void export_archive() {
  sys::tool_run("ipa-export");
}

static void upload_archive(const char * dag) {
  auto method = sys::env("LECO_IOS_METHOD");
  if (0 != strcmp("app-store-connect", method)) return;

  sys::tool_run("ipa-upload", " -i %s -s", dag);
}

int main(int argc, char ** argv) try {
  const char * input {};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      default: usage(); break;
    }
  });
  if (!input || opts.argc != 0) usage();

  auto path = sim::sb { input };
  path.path_parent();
  path /= "export.xcarchive";
  path /= "Products";
  path /= "Applications";
  path /= sim::path_filename(input);
  path.path_extension("app");

  copy("exs", input, *path);
  copy("rsrc", input, *path);
  xcassets(input, *path);

  bool landscape {};
  auto disp_name = sim::copy_path_stem(input);
  sys::dag_read(input, [&](auto id, auto val) {
    switch (id) {
      case 'name': disp_name = sim::sb { val }; break;
      case 'land': landscape = true; break;
      default: break;
    }
  });

  path /= sim::path_filename(input);
  path.path_extension("exe");
  gen_iphone_ipa(*path, *disp_name, landscape);

  export_archive();
  upload_archive(input);

  return 0;
} catch (...) {
  return 1;
}
