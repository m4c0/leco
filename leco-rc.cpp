#pragma leco tool

import gopt;
import mtime;
import sim;
import sys;

static void usage() { sys::die("invalid usage"); }

int main(int argc, char **argv) try {
  const char * input {};

  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i': input = val; break;
    default: usage(); break;
    }
  });
  if (opts.argc != 0) usage();
  if (!input) usage();

  sim::sb rc {};
  sys::dag_read(input, [&](auto id, auto file) {
    switch (id) {
      case 'srcf': rc = sim::sb { file }; break;
      default: break;
    }
  });

  if (rc == "") sys::die("invalid dag file");
  rc.path_extension("rc");
  if (mtime::of(*rc) == 0) return 0;

  sim::sb res { input };
  res.path_extension("res");
  sys::runf("llvm-rc.exe /FO %s %s", *res, *rc);
} catch (...) {
  return 1;
}
