#pragma leco tool

import sys;

int main() try {
  str::set unique_parents {};
  sys::for_each_tag_in_dags('vers', true, [&](auto dag, auto file) {
    auto parent = sim::path_parent(dag);
    parent.path_parent();
    parent.path_parent();
    if (!unique_parents.insert(*parent)) return;
    sys::runf("git -C %s pull", *parent);
  });
} catch (...) {
  return 1;
}
