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

static void compile(const char *src) {
  sys::log("compiling object", src);
  sys::tool_run("clang", "-i %s -t %s %s", src, target, common_flags);
}

static str::set done {};
static void process(const char * dag) {
  if (!done.insert(dag)) return;

  auto mtime = 0ULL;

  sim::sb src {};
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'head': mtime = max(mtime, mtime::of(file)); break;
      case 'mdag': {
        process(file);

        sim::sb pcm { file };
        pcm.path_extension("pcm");
        mtime = max(mtime, mtime::of(*pcm));
        break;
      }
      case 'bdag':
      case 'idag': process(file); break;
      case 'srcf': {
        if (".cppm" == sim::path_extension(file)) {
          src = sim::sb { dag };
          src.path_extension("pcm");
        } else {
          src = sim::sb { file };
        }
        mtime = max(mtime, mtime::of(*src));
        break;
      }
      default: break;
    }
  });

  if (src.len == 0) sys::die("dag without source info: [%s]", dag);

  auto obj = sim::sb { dag };
  obj.path_extension("o");

  if (mtime > mtime::of(*obj)) compile(*src);
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

