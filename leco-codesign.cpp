#pragma leco tool

import jute;
import sys;

static void usage() {
  die(R"(
Invoke Apple's codesign utility to sign a specific directory.

Usage: ../leco/leco.exe codesign [-d <dir>] [-d <dir>]...

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

static pair sign_times(const char * path) {
  pair res {};
  for (auto entry : pprent::list(path)) {
    if (entry[0] == '.') continue;

    auto f = mtime::of(path);
    res.signature = sys::max(res.signature, f);
  }
  return res;
}
static pair mod_times(const char * path) {
  pair res {};

  for (auto entry : pprent::list(path)) {
    if (entry[0] == '.') continue;

    auto p = sim::sb { path } / entry;
    auto ev = jute::view::unsafe(entry);
    auto [f, s] = (ev == "_CodeSignature") ? sign_times(*p) : mod_times(*p);
    res.files = sys::max(res.files, f);
    res.signature = sys::max(res.signature, s);
  }

  auto f = mtime::of(path);
  res.files = res.files < f ? f : res.files;
  return res;
}
static bool sign_is_fresh(const char * path) {
  auto [ f, s ] = mod_times(path);
  return f <= s;
}

static void sign(const char * path) try {
  // Only sign when we want to sign
  if (sign_is_fresh(path)) return;

  sys::log("codesign", path);
  sys::runf("codesign -f -s %s %s", (const char *)sys::envs::ios_team_id(), path);
} catch (...) {
  return;
}

// TODO: make this tool independent (i.e. `leco codesign` without `-d`)
// TODO: take framework signing logic from "exs" (after `fdir` is in dagger)
int main(int argc, char ** argv) try {
  if (!sys::is_tgt_apple()) return 0;

  const auto shift = [&] { return argc > 1 ? (argc--, *++argv) : nullptr; };
  while (auto val = shift()) {
    if ("-d"_s == val) sign(shift());
    else usage();
  }
} catch (...) {
  return 1;
}
