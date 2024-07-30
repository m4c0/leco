#pragma leco tool
#define SIM_IMPLEMENTATION

#include "sim.hpp"

import gopt;
import sys;

void usage() {
  sys::die(R"(
Exports an iOS application - i.e. generates an uploadable IPA.

Usage: ../leco/leco.exe ipa -i <input.dag>

Where:
        -i: Application DAG
)");
}

static const char *build_path{};

static void export_archive() {
  sim_sbt cmd{1024};
  sim_sb_printf(&cmd,
                "xcodebuild -exportArchive"
                " -archivePath %s/export.xcarchive"
                " -exportPath %s/export"
                " -exportOptionsPlist %s/export.plist",
                build_path, build_path, build_path);
  sys::run(cmd.buffer);
}

int main(int argc, char **argv) try {
  const char *input{};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    default:
      usage();
      break;
    }
  });
  if (!input || opts.argc != 0)
    usage();

  sim_sbt build_path{};
  sim_sb_path_copy_parent(&build_path, input);
  ::build_path = build_path.buffer;

  export_archive();

  return 0;
} catch (...) {
  return 1;
}
