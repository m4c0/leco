#pragma leco tool

import sys;

static str::map spec_cache {};
static auto calc_mtime(const char * dag) {
  auto &mtime = spec_cache[dag];
  if (mtime != 0) return mtime;
  mtime = mtime::of(dag);

  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'head': mtime = sys::max(mtime, mtime::of(file)); break;
      case 'mdag': mtime = sys::max(mtime, calc_mtime(file)); break;
    }
  });

  return mtime;
}

int main() try {
  bool dirty = false;
  do {
    spec_cache = {};

    sys::mt mt {};
    sys::for_each_tag_in_dags('pcmf', true, [&](auto dag, auto file) {
      if (calc_mtime(dag) < mtime::of(file)) return;

      bool ok = true;
      sys::dag_read(dag, [&](auto id, auto file) {
        if (id != 'mdag') return;

        auto pcmf = sys::read_dag_tag('pcmf', file);
        if (calc_mtime(file) < mtime::of(*pcmf)) return;

        ok = false;
      });
      if (!ok) {
        dirty = true;
        return;
      }

      sim::sb src = sys::read_dag_tag('srcf', dag);

      auto deps = sim::path_parent(dag) / "deplist";
      auto incs = sim::path_parent(dag) / "includes";
    
      sys::log("compiling module", *src);
      sys::tool_run("clang", "-i %s -- -std=c++2b --precompile -o %s @%s @%s", *src, file, *deps, *incs);
    });
  } while (dirty);
} catch (...) {
  return 1;
}
