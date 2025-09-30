#pragma leco tool
#include <string.h>

import gopt;
import sys;

static void usage() {
  die(R"(
Invoke Apple's codesign utility to sign a specific directory.

Usage: ../leco/leco.exe codesign -d <dir>

Where:
        -d  Directory to sign

This requires an Apple environment (i.e. Apple hardware + Xcode Command Line
Tools) to run.

The team ID should be set as an environment variable named "LECO_IOS_TEAM".
Absence of that will emit a warning and skip the execution. This allows builds
without real signing - which is fine until you use a custom xcframework or if
you want to bundle in an IPA.

If there is an existing signature, the process will be skipped unless signed
files are newer than the signature.
)");
}

struct pair {
  mtime::t files {};
  mtime::t signature {};
};

static pair mod_times(const char * path) {
  pair res {};

  for (auto entry : pprent::list(path)) {
    if (entry[0] == '.') continue;

    auto p = sim::sb { path } / entry;
    auto [ f, s ] = mod_times(*p);
    res.files = res.files < f ? f : res.files;
    res.signature = res.signature < s ? s : res.signature;
  }

  auto f = mtime::of(path);
  res.files = res.files < f ? f : res.files;

  if (strstr(path, "_CodeSignature")) {
    res.signature = res.signature < f ? f : res.signature;
  }
  return res;
}
static bool sign_is_fresh(const char * path) {
  auto [ f, s ] = mod_times(path);
  return f == s;
}

static void sign(const char * path) try {
  // Only sign when we want to sign
  sys::log("codesign", path);
  sys::runf("codesign -f -s %s %s", (const char *)sys::envs::ios_team_id(), path);
} catch (...) {
  return;
}

int main(int argc, char ** argv) try {
  const char * path {};
  auto opts = gopt_parse(argc, argv, "d:", [&](auto ch, auto val) {
    switch (ch) {
      case 'd': path = val; break;
      default: usage();
    }
  });
  if (!path || opts.argc) usage();

  if (sign_is_fresh(path)) return 0;

  sign(path);
} catch (...) {
  return 1;
}
