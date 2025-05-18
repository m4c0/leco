#pragma leco tool

import sys;

int main() try {
  if (!sys::is_tgt_windows()) return 0;

  sys::for_each_dag(false, [](auto * dag, auto id, auto) {
    if (id != 'tapp') return;

    sys::dag_read(dag, [dag](auto id, auto file) {
      if (id != 'srcf') return;

      auto rc = sim::sb { file }.path_extension("rc");
      if (mtime::of(*rc) == 0) return;

      auto res = sim::sb { dag }.path_extension("res");
      sys::run("llvm-rc.exe /FO", *res, *rc);
    });
  });
} catch (...) {
  return 1;
}
