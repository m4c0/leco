#pragma leco tool
#include "sim.hpp"

#include <string.h>

import gopt;
import mtime;
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
static const char * argv0;
static const char * target;

static constexpr auto max(auto a, auto b) { return a > b ? a : b; }

static void compile(const char *src) {
  sys::log("compiling object", src);

  sim_sbt cmd{};
  sim_sb_path_copy_parent(&cmd, argv0);
  sim_sb_path_append(&cmd, "leco-clang.exe");
  sim_sb_printf(&cmd, " -i %s -t %s", src, target);
  sim_sb_concat(&cmd, common_flags);
  sys::run(cmd.buffer);
}

static str::set done {};
static void process(const char * dag) {
  if (!done.insert(dag)) return;

  auto mtime = 0ULL;

  sim_sbt src {};
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'head': mtime = max(mtime, mtime::of(file)); break;
      case 'mdag': {
        process(file);

        sim_sbt pcm {};
        sim_sb_copy(&pcm, dag);
        sim_sb_path_set_extension(&pcm, "pcm");
        mtime = max(mtime, mtime::of(pcm.buffer));
        break;
      }
      case 'bdag':
      case 'idag': process(file); break;
      case 'srcf': {
        if (0 == strcmp(".cppm", sim_path_extension(file))) {
          sim_sb_copy(&src, dag);
          sim_sb_path_set_extension(&src, "pcm");
        } else {
          sim_sb_copy(&src, file);
        }
        mtime = max(mtime, mtime::of(src.buffer));
        break;
      }
      default: break;
    }
  });

  if (src.len == 0) sys::die("dag without source info: [%s]", dag);

  sim_sbt obj {};
  sim_sb_copy(&obj, dag);
  sim_sb_path_set_extension(&obj, "o");

  if (mtime > mtime::of(obj.buffer)) {
    compile(src.buffer);
  }
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

  process(input.buffer);
} catch (...) {
  return 1;
}

