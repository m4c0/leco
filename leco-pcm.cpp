#pragma leco tool

import sys;

static constexpr auto max(auto a, auto b) { return a > b ? a : b; }

static void compile(const char * src, const char * pcm, const char * dag) {
  auto deps = sim::path_parent(dag) / "deplist";
  auto incs = sim::path_parent(dag) / "includes";

  sys::log("compiling module", src);
  sys::tool_run("clang", "-i %s -- -std=c++2b --precompile -o %s @%s @%s", src, pcm, *deps, *incs);
}

static str::map spec_cache {};
static auto calc_mtime(const char * dag) {
  auto &mtime = spec_cache[dag];
  if (mtime != 0) return mtime;
  mtime = mtime::of(dag);

  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'head': mtime = max(mtime, mtime::of(file)); break;
      case 'mdag': mtime = max(mtime, calc_mtime(file)); break;
    }
  });

  return mtime;
}
static void compile(const char * dag) {
  sim::sb src {};
  sim::sb pcm {};
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'srcf': src = sim::sb { file }; break;
      case 'pcmf': pcm = sim::sb { file }; break;
    }
  });
  if (calc_mtime(dag) < mtime::of(*pcm)) return;

  compile(*src, *pcm, dag);
}

int main() try {
  sys::for_each_dag(true, [](auto dag, auto id, auto file) {
    if (id == 'pcmf') compile(dag);
  });
} catch (...) {
  return 1;
}
