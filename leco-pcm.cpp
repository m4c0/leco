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
static auto process_spec(const char * dag) {
  auto &mtime = spec_cache[dag];
  if (mtime != 0) return mtime;
  mtime = 1;

  sim::sb src {};
  sim::sb pcm {};
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'head': mtime = max(mtime, mtime::of(file)); break;
      case 'mdag': mtime = max(mtime, process_spec(file)); break;

      case 'srcf': src = sim::sb { file }; break;
      case 'pcmf': pcm = sim::sb { file }; break;
      default: break;
    }
  });

  if (!src.len) sys::die("missing source for [%s]", dag);

  if (!pcm.len) return mtime;
  if (sim::path_extension(*src) != ".cppm") return mtime;

  mtime = max(mtime, mtime::of(*src));

  if (mtime > mtime::of(*pcm)) {
    compile(*src, *pcm, dag);
    mtime = mtime::of(*pcm);
  }

  return mtime;
}

int main() try {
  // Caches globally so we don't reprocess DAGs for each root
  str::set cache {};
  sys::for_each_root_dag([&](auto dag, auto, auto) {
    process_spec(dag);

    // Search for imports starting from an implementation file.
    sys::recurse_dag(&cache, dag, [&](auto dag, auto id, auto file) {
      if (id == 'mdag') process_spec(file);
    });
  });
} catch (...) {
  return 1;
}
