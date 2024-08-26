#pragma leco tool
#include "sim.hpp"

#include <string.h>

import gopt;
import mtime;
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
static const char * argv0;
static const char * target;

static constexpr auto max(auto a, auto b) { return a > b ? a : b; }

static void compile(const char *src) {
  sys::log("compiling module", src);

  sim_sbt cmd{};
  sim_sb_path_copy_parent(&cmd, argv0);
  sim_sb_path_append(&cmd, "leco-clang.exe");
  sim_sb_printf(&cmd, " -i %s -t %s", src, target);
  sim_sb_concat(&cmd, common_flags);
  sys::run(cmd.buffer);
}

static str::map spec_cache {};
static auto process_spec(const char * dag) {
  auto &mtime = spec_cache[dag];
  if (mtime != 0) return mtime;
  mtime = 1;

  sim_sbt src {};

  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'head': mtime = max(mtime, mtime::of(file)); break;
      case 'bdag':
      case 'mdag': mtime = max(mtime, process_spec(file)); break;
      case 'srcf': sim_sb_copy(&src, file); break;
      default: break;
    }
  });

  if (src.len == 0) sys::die("missing source for [%s]", dag);

  if (0 != strcmp(".cppm", sim_sb_path_extension(&src))) return mtime;

  mtime = max(mtime, mtime::of(src.buffer));

  sim_sbt pcm {};
  sim_sb_copy(&pcm, dag);
  sim_sb_path_set_extension(&pcm, "pcm");

  if (mtime > mtime::of(pcm.buffer)) {
    compile(src.buffer);
    mtime = mtime::of(pcm.buffer);
  }

  return mtime;
}

static str::set impl_cache {};
static void process_impl(const char * dag) {
  if (!impl_cache.insert(dag)) return;

  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'bdag':
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
  sim_sbt flags {};
  sim_sbt input {};
  auto opts = gopt_parse(argc, argv, "i:gO", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': sim_sb_path_copy_real(&input, val); break;
      case 'g': sim_sb_concat(&flags, " -g"); break;
      case 'O': sim_sb_concat(&flags, " -O"); break;
      default: usage();
    }
  });

  if (input.len == 0 || opts.argc != 0) usage();

  argv0 = argv[0];
  common_flags = flags.buffer;

  sim_sbt d {};
  sim_sb_path_copy_parent(&d, input.buffer);
  target = sim_sb_path_filename(&d);

  process_spec(input.buffer);
  process_impl(input.buffer);
} catch (...) {
  return 1;
}
