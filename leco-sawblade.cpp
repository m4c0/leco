#pragma leco tool
#include "dag2.hpp"
#include "in2out.hpp"
#include "sim.hpp"

import gopt;
import mtime;
import strset;
import sys;

static bool verbose {};
static const char * argv0;
static const char * target;

static void usage() {
  sys::die(R"(
Recursively updates DAG files as needed

Usage: ../leco/leco sawblade -i <source> -t <triple> [-v]

Where:
        -i: input source file
        -t: target triple
        -v: enable verbose mode
)");
}

static void dagger(const char * src, const char * dag) {
  if (mtime::of(dag) > mtime::of(src)) return;

  if (verbose) sys::log("inspecting", src);

  sim_sbt cmd { 10240 };
  sim_sb_path_copy_parent(&cmd, argv0);
  sim_sb_path_append(&cmd, "leco-dagger.exe");
  sim_sb_concat(&cmd, " -t ");
  sim_sb_concat(&cmd, target);
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, src);
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, dag);
  sys::run(cmd.buffer);

  if (mtime::of(dag) == 0)
    sys::die("failed to preprocess [%s]", src);
}

static str::set done {};
static void process(const char * src) {
  if (!done.insert(src)) return;

  sim_sbt dag {};
  in2out(src, &dag, "dag", target);

  dagger(src, dag.buffer);

  dag_read(dag.buffer, [](auto id, auto file) {
    switch (id) {
      case 'bdep':
      case 'impl':
      case 'mdep':
        process(file);
        break;
    }
  });
}

int main(int argc, char ** argv) try {
  argv0 = argv[0];

  const char * input {};

  auto opts = gopt_parse(argc, argv, "i:t:v", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      case 't': target = val; break;
      case 'v': verbose = true; break;
      default: usage();
    }
  });

  if (!input || !target || opts.argc != 0) usage();

  sim_sbt in {};
  sim_sb_path_copy_real(&in, input);
  process(in.buffer);
} catch (...) {
  return 1;
}