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

static str::set done {};
static void process(const char * dag) {
  if (!done.insert(dag)) return;

  auto mtime = 0ULL;

  sim::sb src {};
  sim::sb pcm {};
  sim::sb obj {};
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'head': mtime = max(mtime, mtime::of(file)); break;
      case 'idag': process(file); break;
      case 'mdag': {
        process(file);

        sim::sb pcm { file };
        pcm.path_extension("pcm");
        mtime = max(mtime, mtime::of(*pcm));
        break;
      }

      case 'srcf': src = sim::sb { file }; break;
      case 'pcmf': pcm = sim::sb { file }; break;
      case 'objf': obj = sim::sb { file }; break;
      default: break;
    }
  });

  if (pcm.len > 0) return;
  mtime = max(mtime, mtime::of(*src));

  if (src.len == 0) sys::die("dag without source info: [%s]", dag);
  if (obj.len == 0) sys::die("dag without object info: [%s]", dag);

  if (mtime > mtime::of(*obj)) {
    compile(*src, *obj, dag);
  }
}

int main() try {
  sys::for_each_root_dag([](auto * dag, auto id, auto file) {
    process(dag);
  });
} catch (...) {
  return 1;
}

