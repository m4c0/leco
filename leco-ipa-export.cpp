#pragma leco tool

#include "targets.hpp"

import sim;
import sys;

void usage() {
  sys::die(R"(
Exports an iOS application - i.e. generates an uploadable IPA. This should be
the final step of IPA export - the tool assumes everything else was generated,
signed, etc.

Must be run from the root of the source repository.

It only support iPhoneOS exports. No support for other Apple OSs like WatchOS,
etc.

If you see werid messages from XCode about missing iOS Simulator stuff, you
need to download the iOS SDK. This command might help:

        xcodebuild -downloadPlatform iOS

Usage: ../leco/leco.exe ipa-export

)");
}

static void export_archive(const char * build_path) {
  sys::log("exporting from", build_path);

  sys::runf("xcodebuild -exportArchive"
            " -archivePath %s/export.xcarchive"
            " -exportPath %s/export"
            " -exportOptionsPlist %s/export.plist",
            build_path, build_path, build_path);
}

int main(int argc, char ** argv) try {
  auto path = "."_real / "out" / TGT_IPHONEOS;
  export_archive(path.buffer);
  return 0;
} catch (...) {
  return 1;
}
