#pragma leco tool

import sys;

// TODO: investigate why changes in deps are not triggering these
// Example: change "sys.cppm", no tool gets its obj compiled

static sys::mt mt {};

int main() try {
  sys::for_each_tag_in_dags('pcmf' , true, [](auto * dag, auto file) {
    auto pcm = file;
    auto obj = sys::read_dag_tag('objf', dag);
    if (mtime::of(pcm) < mtime::of(*obj)) return;

    auto deps = "@"_s + *(sim::path_parent(dag) / "deplist");

    auto clang = sys::tool_cmd("clang");
    mt.run("compiling mod obj", *obj, {
      .cmd = clang + " -- -c " + pcm + " -o " + *obj + " " + *deps,
      .proc = new p::proc { *clang, "--", "-c", pcm, "-o", *obj, *deps },
    });
  });
} catch (...) {
  return 1;
}
