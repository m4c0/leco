#pragma leco tool

import popen;
import sys;

// TODO: investigate why changes in deps are not triggering these
// Example: change "sys.cppm", no tool gets its obj compiled

static sys::mt mt {};

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

static void compile_objf(const char * dag, const char * _) {
  if (sys::read_dag_tag('pcmf', dag) != "") return;

  auto src = sys::read_dag_tag('srcf', dag);
  auto obj = sys::read_dag_tag('objf', dag);
  if (calc_mtime(dag) < mtime::of(*obj)) return;

  auto ext = sim::path_extension(*src);
  auto lang = "-std=c++2b";
  if (ext == ".m" || ext == ".mm") lang = "-fmodules -fobjc-arc";
  else if (ext == ".c") lang = "-std=c11";
 
  auto deps = "@"_s + *(sim::path_parent(dag) / "deplist");
  auto incs = "@"_s + *(sim::path_parent(dag) / "includes");

  auto i = mt.reserve();

  sys::log("compiling object", *obj);
  auto clang = sys::tool_cmd("clang");
  mt.run(i, {
    .cmd = clang + " -i " + *src + " -- " + lang + " -c -o " + *obj + " " + *deps + " " + *incs,
    .proc = new p::proc { *clang, "-i", *src, "--", lang, "-c", "-o", *obj, *deps, *incs },
  });
}

int main() try {
  sys::for_each_tag_in_dags('objf', true, &compile_objf);
} catch (...) {
  return 1;
}

