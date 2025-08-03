#pragma leco tool

import sys;

int main() try {
  sys::for_each_root_dag([](auto dag, auto id, auto file) {
    if (id != 'test') return;
    sys::log("running", file);
    sys::run(file);
  });
} catch (...) {
  return 1;
}
