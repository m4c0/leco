#pragma leco tool

import sys;

// TODO: investigate why changes in deps are not triggering these
// Example: change "sys.cppm", no tool gets its obj compiled
int main() try {
  sys::for_each_tag_in_dags('pcmf' , true, [](auto * dag, auto file) {
    sim::sb pcm { file };
    sim::sb obj = sys::read_dag_tag('objf', dag);
    if (mtime::of(*pcm) < mtime::of(*obj)) return;

    auto deps = sim::path_parent(dag) / "deplist";

    sys::log("compiling mod obj", *obj);
    sys::tool_run("clang", "-- -c %s -o %s @%s", *pcm, *obj, *deps);
  });
} catch (...) {
  return 1;
}
