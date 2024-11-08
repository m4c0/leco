#pragma leco tool

#include <errno.h>
#include <stdio.h>
#include <string.h>

import gopt;
import mtime;
import pprent;
import sim;
import strset;
import sys;

static bool verbose {};
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

  sys::tool_run("dagger", "-t %s -i %s -o %s", target, src, dag);
  if (mtime::of(dag) == 0)
    sys::die("failed to preprocess [%s]", src);
}

static str::set done {};
static void process(const char * src) {
  if (!done.insert(src)) return;

  auto dag = sim::path_parent(src) / "out" / target / sim::path_filename(src);
  dag.path_extension("dag");

  dagger(src, *dag);

  sys::dag_read(*dag, [](auto id, auto file) {
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
  sim::sb input {};

  auto opts = gopt_parse(argc, argv, "i:t:v", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = sim::path_real(val); break;
      case 't': target = val; break;
      case 'v': verbose = true; break;
      default: usage();
    }
  });

  if (!target || opts.argc != 0) usage();

  if (input.len) {
    process(*input);
    return 0;
  }

  for (auto file : pprent::list(".")) {
    auto ext = sim::path_extension(file);
    if (!ext.len) continue;

    if (ext != ".cppm" && ext != ".cpp" && ext != ".c") continue;

    auto in = sim::path_real(file);
    process(*in);

    errno = 0;
  }
  if (errno) {
    perror("could not list current directory");
    throw 0;
  }
} catch (...) {
  return 1;
}
