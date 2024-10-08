#pragma leco tool

#include "sim.hpp"
#include "targets.hpp"

#include <string.h>

import gopt;
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

  sim_sbt ipa {};
  sim_sb_path_copy_parent(&ipa, input);
  if (!IS_TGT(TGT_IPHONEOS, sim_sb_path_filename(&ipa)))
    sys::die("only iPhone target is supported");

  sim_sb_path_append(&ipa, "export");
  sim_sb_path_append(&ipa, sim_path_filename(input));
  sim_sb_path_set_extension(&ipa, "ipa");

  const char * verb = upload ? "--upload-app" : "--validate-app";

  sim_sbt cmd { 10240 };
  sim_sb_printf(&cmd, 
      "xcrun altool %s -t iphoneos -f %s --apiKey %s --apiIssuer %s",
      verb, ipa.buffer, sys::env("LECO_IOS_API_KEY"), sys::env("LECO_IOS_API_ISSUER"));
  sys::run(cmd.buffer);
} catch (...) {
  return 1;
}
