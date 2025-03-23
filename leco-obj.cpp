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

Usage: ../leco/leco.exe obj -i <input.dag> -g -O

Where:
        -i: input DAG
        -g: enable debug symbols
        -O: enable optimisations
)");
}

static const char * common_flags;
static const char * target;

static constexpr auto max(auto a, auto b) { return a > b ? a : b; }

static void deplist(const char * dag) {
  // TODO: check if dag is newer?
  sys::tool_run("deplist", "-i %s", dag);
}
static void compile(const char * src, const char * obj, const char * deps) {
  auto ext = sim::path_extension(src);
  const char * lang = "-std=c++2b";
  if (ext == ".m" || ext == ".mm") lang = "-fmodules -fobjc-arc";
  else if (ext == ".c") lang = "-std=c11";
 
  // TODO: find a way to avoid using -I when compiling pcm to obj
  sys::log("compiling object", src);
  sys::tool_run("clang", "-i %s -t %s %s -- %s -c -o %s @%s", src, target, common_flags, lang, obj, deps);
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

    deplist(dag);
    compile(*src, *obj, *deps);
  }
}

int main(int argc, char ** argv) try {
  sim::sb flags {};
  sim::sb input {};
  auto opts = gopt_parse(argc, argv, "i:gO", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = sim::path_real(val); break;
      case 'g': flags += " -g"; break;
      case 'O': flags += " -O"; break;
      default: usage();
    }
  });

  if (input.len == 0 || opts.argc != 0) usage();

  common_flags = *flags;

  auto d = sim::path_parent(*input);
  target = d.path_filename();

  process(*input);
} catch (...) {
  return 1;
}

