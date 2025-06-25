#pragma leco tool

import sys;

int main() try {
  if (!sys::is_tgt_windows()) return 0;

  sys::for_each_tag_in_dags('tapp', false, [](auto * dag, auto) {
    auto rc = sys::read_dag_tag('srcf', dag).path_extension("rc");
    if (mtime::of(*rc) == 0) return;

    auto res = sim::sb { dag }.path_extension("res");
    sys::runf("llvm-rc.exe /FO %s %s", *res, *rc);
  });
} catch (...) {
  return 1;
}
