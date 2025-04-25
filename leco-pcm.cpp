#pragma leco tool

import mtime;
import sim;
import strset;
import sys;

static constexpr auto max(auto a, auto b) { return a > b ? a : b; }

static void compile(const char * src, const char * pcm, const char * dag) {
  auto deps = sim::sb { dag };
  deps.path_extension("deps");
  auto incs = sim::sb { dag };
  incs.path_extension("incs");

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

// Caches globally so we don't reprocess DAGs for each root
static str::set cache {};
void process(const char * dag) {
  process_spec(dag);

  // Search for imports starting from an implementation file.
  sys::recurse_dag(&cache, dag, [&](auto dag, auto id, auto file) {
    if (id == 'mdag') process_spec(file);
  });
}

int main() try {
  sys::for_each_dag(sys::target(), false, [](auto * dag, auto id, auto file) {
    switch (id) {
      case 'tapp':
      case 'tdll':
      case 'tool':
      case 'tmmd': process(dag); break;
      default: break;
    }
  });
} catch (...) {
  return 1;
}
