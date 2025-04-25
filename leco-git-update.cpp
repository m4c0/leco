#pragma leco tool

import sim;
import strset;
import sys;

int main() try {
  str::set unique_parents {};
  sys::for_each_dag(sys::target(), true, [&](auto dag, auto id, auto file) {
    if (id != 'vers') return;

    auto parent = sim::path_parent(dag);
    parent.path_parent();
    parent.path_parent();
    if (!unique_parents.insert(*parent)) return;
    sys::runf("git -C %s pull", *parent);
  });
} catch (...) {
  return 1;
}
