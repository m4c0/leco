#pragma leco tool
#include <stdio.h>

import gopt;
import mtime;
import sim;
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
      sim::sb js { file };
      js.path_extension("js");
      if (mtime::of(*js) > 0) concat(out, *js);
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
        sim::sb js { file };
        js.path_extension("js");
        mtime = max(mtime, mtime::of(*js));
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
  sim::sb appdir {};

  auto opts = gopt_parse(argc, argv, "i:a:", [&](auto ch, auto val) {
    switch (ch) {
    case 'a': appdir = sim::path_real(val); break;
    case 'i': input = val; break;
    default:  usage();
    }
  });

  if (opts.argc != 0 || !input || !appdir.len)
    usage();

  appdir /= "leco.js";
  if (mtime::of(*appdir) >= mtime_rec(input)) return 0;

  sys::log("generating", *appdir);

  auto f = sys::fopen(*appdir, "wb");
  fprintf(f, "var leco_exports;\n");
  fprintf(f, "var leco_imports = {};\n");

  concat_all(f, input);

  concat(f, "../leco/wasm.js");
  fclose(f);
}
