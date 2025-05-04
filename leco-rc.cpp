#pragma leco tool

import sys;

void run(const char * dag, const char * file) {
  sim::sb rc { file };
  rc.path_extension("rc");
  if (mtime::of(*rc) == 0) return;

  sim::sb res { dag };
  res.path_extension("res");
  sys::runf("llvm-rc.exe /FO %s %s", *res, *rc);
}

int main() try {
  if (!sys::is_tgt_windows()) return 0;

  sys::for_each_dag(false, [](auto * dag, auto id, auto file) {
    if (id != 'tapp') return;

    sys::dag_read(dag, [&](auto id, auto file) {
      if (id == 'srcf') run(dag, file);
    });
  });
} catch (...) {
  return 1;
}
