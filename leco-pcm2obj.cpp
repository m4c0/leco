#pragma leco tool

import sys;

// TODO: investigate why changes in deps are not triggering these
// Example: change "sys.cppm", no tool gets its obj compiled
int main() try {
  sys::for_each_dag(true, [](auto * dag, auto id, auto file) {
    if (id != 'pcmf') return;

    sim::sb pcm { file };
    sim::sb obj {};
    sys::dag_read(dag, [&](auto id, auto file) {
      if (id == 'objf') obj = sim::sb { file };
    });
    if (mtime::of(*pcm) < mtime::of(*obj)) return;

    auto deps = sim::path_parent(dag) / "deplist";

    sys::log("compiling mod obj", *obj);
    sys::tool_run("clang", "-- -c %s -o %s @%s", *pcm, *obj, *deps);
  });
} catch (...) {
  return 1;
}
