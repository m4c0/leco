#pragma leco tool

#include "targets.hpp"

#include <string.h>

import gopt;
import sim;
import sys;

void usage() {
  sys::die(R"(
Uploads an iOS application. This assumes everything else was generated, signed,
exported, etc.

Must be run from the root of the source repository.

It only support iPhoneOS exports. No support for other Apple OSs like WatchOS,
etc.

Usage: ../leco/leco.exe ipa-upload -i <input-dag> [-s]

Where:
        -i: input dag
        -s: submit (defaults to validate)

)");
}

int main(int argc, char ** argv) try {
  const char * input {};
  bool upload {};
  auto opts = gopt_parse(argc, argv, "i:s", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      case 's': upload = true; break;
      default: usage();
    }
  });
  if (opts.argc != 0 || !input) usage();

  auto ipa = sim::path_parent(input);
  if (!IS_TGT(TGT_IPHONEOS, ipa.path_filename()))
    sys::die("only iPhone target is supported");

  ipa = ipa / "export" / sim::path_filename(input);
  ipa.path_extension("ipa");

  const char * verb = upload ? "--upload-app" : "--validate-app";

  sys::runf(
      "xcrun altool %s -t iphoneos -f %s --apiKey %s --apiIssuer %s",
      verb, ipa.buffer, sys::env("LECO_IOS_API_KEY"), sys::env("LECO_IOS_API_ISSUER"));
} catch (...) {
  return 1;
}
