#pragma leco tool
#include <string.h>

import gopt;
import mtime;
import pprent;
import sim;
import sys;

static void usage() { sys::die("invalid usage"); }

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

  sys::log("codesign", path);
  sys::runf("codesign -f -s %s %s", sys::env("LECO_IOS_TEAM"), path);
} catch (...) {
  return 1;
}