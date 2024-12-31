#pragma leco tool

#include <stdio.h>
#include <stdint.h>
#include <string.h>

import gopt;
import mtime;
import sim;
import strset;
import sys;

static str::set added{};

static const char *target{};
static const char *resdir{};

static void usage() { sys::die("invalid usage"); }

static void copy_res(const char *file) {
  auto path = sim::sb { resdir } / sim::path_filename(file);
  if (mtime::of(*path) >= mtime::of(file)) return;

  sys::log("hard-linking", file);
  sys::link(file, *path);
}

// TODO: new utility for shader compilation
static bool must_recompile(const char * file, auto spv_time) {
  if (spv_time < mtime::of(file)) return true;

  char buf[10240];
  auto f = sys::fopen(file, "r");
  unsigned line { 0 };
  while (!feof(f) && fgets(buf, sizeof(buf), f)) {
    line++;

    auto si = strstr(buf, "#include ");
    if (!si) continue;

    auto fail = [&](auto at, auto msg) {
      int col = at - buf + 1;
      sys::die("%s:%d:%d: %s", file, line, col, msg);
    };

    auto s = strchr(buf, '"');
    if (!s) fail(si, "invalid include directive");
    auto e = strchr(++s, '"');
    if (!e) fail(s, "unclosed include directive");

    *e = 0;
    auto path = sim::path_parent(file) / s;
    if (!mtime::of(*path)) fail(s, "include file not found");
    if (must_recompile(*path, spv_time)) return true;
  }

  fclose(f);

  return false;
}
static void copy_shader(const char *file) {
  auto out = sim::sb { resdir } / sim::path_filename(file) + ".spv";
  if (!must_recompile(file, mtime::of(*out))) return;

  sys::log("compiling shader", file);
  // TODO: suppress "ERROR: " from glslangValidator's output
  sys::runf("glslangValidator --target-env spirv1.3 -V -o %s %s", *out, file);
}

static void read_dag(const char *dag) {
  if (!added.insert(dag)) return;

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'rsrc': copy_res(file); break;
      case 'shdr': copy_shader(file); break;
      case 'idag':
      case 'mdag': read_dag(file); break;
      default: break;
    }
  });
}

int main(int argc, char **argv) try {
  const char *input{};
  auto opts = gopt_parse(argc, argv, "o:i:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      case 'o': resdir = val; break;
      default: usage();
    }
  });

  if (!input || !resdir) usage();
  if (opts.argc != 0) usage();

  sys::mkdirs(resdir);

  auto path = sim::path_parent(input);
  target = path.path_filename();

  read_dag(input);

  return 0;
} catch (...) {
  return 1;
}
