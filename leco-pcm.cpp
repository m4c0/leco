#pragma leco tool

import gopt;
import mtime;
import sim;
import strset;
import sys;

static void usage() {
  sys::die(R"(
Compiles PCMs recursively.

Usage: ../leco/leco.exe pcm -i <input.dag> -g -O

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
  sys::log("compiling module", src);
  sys::tool_run("clang", "-i %s -t %s %s", src, target, common_flags);
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
    compile(*src);
    mtime = mtime::of(*pcm);
  }

  return mtime;
}

static str::set impl_cache {};
static void process_impl(const char * dag) {
  if (!impl_cache.insert(dag)) return;

  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'mdag':
        process_spec(file);
        process_impl(file);
        break;
      case 'idag': process_impl(file); break;
      default: break;
    }
  });

  return;
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

  process_spec(*input);
  process_impl(*input);
} catch (...) {
  return 1;
}
