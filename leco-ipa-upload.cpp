#pragma leco tool

import sys;

void usage() {
  die(R"(
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
  // TODO: match with dag target or transform this into a root-diver
  if (!sys::is_tgt_iphoneos()) die("only iPhone target is supported");

  const auto shift = [&] { return argc > 1 ? (argc--, *++argv) : nullptr; };

  const char * input {};
  bool upload {};
  while (auto val = shift()) {
    if      ("-i"_s == val) input = shift();
    else if ("-s"_s == val) upload = true;
    else usage();
  }
  if (!input) usage();

  auto ipa = sim::path_parent(input);

  ipa = ipa / "export" / sim::path_filename(input);
  ipa.path_extension("ipa");

  const char * verb = upload ? "--upload-app" : "--validate-app";

  sys::runf(
      "xcrun altool %s -t iphoneos -f %s --apiKey %s --apiIssuer %s",
      verb, ipa.buffer,
      (const char *)sys::envs::ios_api_key(),
      (const char *)sys::envs::ios_api_issuer());
} catch (...) {
  return 1;
}
