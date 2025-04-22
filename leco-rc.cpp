#pragma leco tool

import gopt;
import mtime;
import sim;
import sys;

static void usage() { sys::die("invalid usage"); }

void run(const char * dag) {
  sim::sb rc {};
  sys::dag_read(dag, [&](auto id, auto file) {
    if (id == 'srcf') rc = sim::sb { file };
  });

  if (rc == "") sys::die("dag file without source information");
  rc.path_extension("rc");
  if (mtime::of(*rc) == 0) return;

  sim::sb res { dag };
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
