#pragma leco tool

import sys;

static constexpr auto max(auto a, auto b) { return a > b ? a : b; }

static void compile(const char * src, const char * obj, const char * dag) {
  auto ext = sim::path_extension(src);
  const char * lang = "-std=c++2b";
  if (ext == ".m" || ext == ".mm") lang = "-fmodules -fobjc-arc";
  else if (ext == ".c") lang = "-std=c11";
 
  auto deps = sim::path_parent(dag) / "deplist";
  auto incs = sim::path_parent(dag) / "includes";

  sys::log("compiling object", obj);
  sys::tool_run("clang", "-i %s -- %s -c -o %s @%s @%s", src, lang, obj, *deps, *incs);
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
  bool pcm = false;
  sim::sb src {};
  sim::sb obj {};
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'pcmf': pcm = true; break;
      case 'srcf': src = sim::sb { file }; break;
      case 'objf': obj = sim::sb { file }; break;
    }
  });
  if (pcm) return;
  if (calc_mtime(dag) < mtime::of(*obj)) return;

  compile(*src, *obj, dag);
}

int main() try {
  sys::for_each_dag(true, [](auto * dag, auto id, auto file) {
    if (id == 'objf') compile(dag);
  });
} catch (...) {
  return 1;
}

