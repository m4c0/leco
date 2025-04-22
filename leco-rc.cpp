#pragma leco tool

import gopt;
import mtime;
import sim;
import sys;

static void usage() { sys::die("invalid usage"); }

void run(const char * input) {
  sim::sb rc {};
  sys::dag_read(input, [&](auto id, auto file) {
    switch (id) {
      case 'srcf': rc = sim::sb { file }; break;
      default: break;
    }
  });

  if (rc == "") sys::die("invalid dag file");
  rc.path_extension("rc");
  if (mtime::of(*rc) == 0) return;

  sim::sb res { input };
  res.path_extension("res");
  sys::runf("llvm-rc.exe /FO %s %s", *res, *rc);
}

int main(int argc, char **argv) try {
  if (argc != 1) usage();
  if (!sys::is_tgt_windows(sys::target())) return 0;

  sys::for_each_dag(sys::target(), false, [](auto * dag, auto id, auto file) {
    if (id == 'tadd') run(dag);
  });
} catch (...) {
  return 1;
}
