#pragma leco tool

import sys;

int main() try {
  // Only makes sense if we can run those tests
  if (!sys::is_tgt_host()) return 0;

  sys::for_each_root_dag([](auto dag, auto id, auto file) {
    if (id != 'tprt') return;
    sys::log("running", file);
    sys::run(file);
  });
  sys::for_each_root_dag([](auto dag, auto id, auto file) {
    if (id != 'test') return;
    sys::log("running", file);
    sys::run(file);
  });
} catch (...) {
  return 1;
}
