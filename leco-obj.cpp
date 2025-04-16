#pragma leco tool

#include <string.h>

import gopt;
import mtime;
import sim;
import strset;
import sys;

static void usage() {
  sys::die(R"(
Compiles object files recursively.

Usage: ../leco/leco.exe obj -t <target>
Where: -t: target triple
)");
}

static const char * target = sys::host_target;

static constexpr auto max(auto a, auto b) { return a > b ? a : b; }

static void compile(const char * src, const char * obj, const char * deps) {
  auto ext = sim::path_extension(src);
  const char * lang = "-std=c++2b";
  if (ext == ".m" || ext == ".mm") lang = "-fmodules -fobjc-arc";
  else if (ext == ".c") lang = "-std=c11";
 
  // TODO: find a way to avoid using -I when compiling pcm to obj
  sys::log("compiling object", src);
  sys::tool_run("clang", "-i %s -t %s -- %s -c -o %s @%s", src, target, lang, obj, deps);
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

  if (pcm.len > 0) src = pcm;
  mtime = max(mtime, mtime::of(*src));

  if (src.len == 0) sys::die("dag without source info: [%s]", dag);
  if (obj.len == 0) sys::die("dag without object info: [%s]", dag);

  if (mtime > mtime::of(*obj)) {
    auto deps = sim::sb { dag };
    deps.path_extension("deps");
    compile(*src, *obj, *deps);
  }
}

int main(int argc, char ** argv) try {
  auto opts = gopt_parse(argc, argv, "t:", [&](auto ch, auto val) {
    if (ch == 't') target = val;
    else usage();
  });
  if (opts.argc) usage();

  sys::for_each_dag(target, false, [](auto * dag, auto id, auto file) {
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

