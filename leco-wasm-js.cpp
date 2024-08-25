#pragma leco tool
#include "sim.hpp"

#include <stdio.h>

import gopt;
import mtime;
import strset;
import sys;

// TODO: check if files actually need regen

static void usage() { sys::die("invalid usage"); }

static void concat(FILE *out, const char *in_file) {
  auto in = sys::fopen(in_file, "rb");

  char buf[10240];
  int got{};
  while ((got = fread(buf, 1, sizeof(buf), in)) > 0) {
    fwrite(buf, 1, got, out);
  }

  fclose(in);
}

static str::set added{};
static void concat_all(FILE *out, const char *dag) {
  if (!added.insert(dag))
    return;

  sys::dag_read(dag, [=](auto id, auto file) {
    switch (id) {
    case 'srcf': {
      sim_sbt js{};
      sim_sb_copy(&js, file);
      sim_sb_path_set_extension(&js, "js");
      if (mtime::of(js.buffer) > 0) {
        sys::log("merging", js.buffer);
        concat(out, js.buffer);
      }
      break;
    }
    case 'idag':
    case 'mdag':
      concat_all(out, file);
      break;
    }
  });
}

static constexpr auto max(auto a, auto b) { return a > b ? a : b; }
static str::map mtime_cache {};
static auto mtime_rec(const char * dag) {
  auto & mtime = mtime_cache[dag];
  if (mtime != 0) return mtime;
  mtime = 1;

  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'srcf': {
        sim_sbt js {};
        sim_sb_copy(&js, file);
        sim_sb_path_set_extension(&js, "js");
        mtime = max(mtime, mtime::of(js.buffer));
        break;
      }
      case 'idag':
      case 'mdag':
        mtime = max(mtime, mtime_rec(file));
        break;
    }
  });

  return mtime;
}

int main(int argc, char **argv) {
  const char *input;
  sim_sbt appdir{};

  auto opts = gopt_parse(argc, argv, "i:a:", [&](auto ch, auto val) {
    switch (ch) {
    case 'a':
      sim_sb_path_copy_real(&appdir, val);
      break;
    case 'i':
      input = val;
      break;
    default:
      usage();
    }
  });

  if (opts.argc != 0 || !input || !appdir.len)
    usage();

  sim_sb_path_append(&appdir, "leco.js");
  if (mtime::of(appdir.buffer) >= mtime_rec(input)) return 0;

  sys::log("generating", appdir.buffer);

  auto f = sys::fopen(appdir.buffer, "wb");
  fprintf(f, "var leco_exports;\n");
  fprintf(f, "var leco_imports = {};\n");

  concat_all(f, input);

  concat(f, "../leco/wasm.js");
  fclose(f);
}
