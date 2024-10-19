#pragma leco tool

import gopt;
import sim;
import sys;

void usage() { sys::die("invalid usage"); }

int main(int argc, char ** argv) try {
  const char * path {};
  auto opts = gopt_parse(argc, argv, "d:", [&](auto ch, auto val) {
    switch (ch) {
      case 'd': path = val; break;
      default: usage();
    }
  });
  if (!path || opts.argc) usage();

  sys::log("codesign", path);
  sys::runf("codesign -f -s %s %s", sys::env("LECO_IOS_TEAM"), path);
} catch (...) {
  return 1;
}
