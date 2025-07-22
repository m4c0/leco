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
  while (true) {
    spec_cache = {};

    bool has_pending_work = false;
    sys::mt mt {};
    sys::for_each_tag_in_dags('pcmf', true, [&](auto dag, auto file) {
      if (calc_mtime(dag) < mtime::of(file)) return;

      has_pending_work = true;

      bool deps_ok = true;
      sys::dag_read(dag, [&](auto id, auto file) {
        if (id != 'mdag') return;

        auto pcmf = sys::read_dag_tag('pcmf', file);
        if (calc_mtime(file) < mtime::of(*pcmf)) return;

        deps_ok = false;
      });
      if (!deps_ok) return;

      sim::sb src = sys::read_dag_tag('srcf', dag);

      auto deps = "@"_s + *(sim::path_parent(dag) / "deplist");
      auto incs = "@"_s + *(sim::path_parent(dag) / "includes");

      mt.run_clang(
          "compiling module", *src,
          "-i", *src, "--", "-std=c++2b", "--precompile", "-o", file, *deps, *incs);
    });

    if (!has_pending_work) break;
  }
} catch (...) {
  return 1;
}
