#pragma leco tool

#include "../mct/mct-syscall.h"
#include <stdio.h>
#include <string.h>

import popen;
import sys;

// Compiles shaders referenced by their modules via pragmas.
// 
// Requires DAGs created via leco-dagger.

static bool must_recompile(const char * file, auto spv_time) {
  if (spv_time < mtime::of(file)) return true;

  char buf[10240];
  auto f = sys::file { file, "r" };
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

  return false;
}
static void build_shader(const char * dag, const char * file) {
  auto out = sim::path_parent(dag) / sim::path_filename(file) + ".spv";
  if (!must_recompile(file, mtime::of(*out))) return;

  sys::log("compiling shader", file);
  p::proc p { "glslangValidator", "--target-env", "spirv1.3", "-V", "-o", *out, file };

  while (p.gets()) {
    auto line = p.last_line_read();
    if (0 == strncmp(line, "ERROR: ", 7))  {
      if (line[7] == '/') line += 7;
      else if (line[8] == ':') line += 7;
    }
    putln(line);
  }

  if (p.wait() != 0) sys::die("shader compilation failed");

  if (!sys::is_tgt_wasm()) return;

  auto gles = sim::path_parent(dag) / sim::path_filename(file) + ".gles";
  const char * args[] { "spirv-cross", "--es", "--version", "300", "--output", *gles, *out, 0 };
  if (0 != mct_syscall_spawn(args[0], args)) sys::die("shader cross to GLES failed");
}

static void run(const char * dag, const char * _) {
  sys::recurse_dag(dag, [&](auto dag, auto id, auto file) {
    if (id == 'shdr') build_shader(dag, file);
  });
}

int main() try {
  sys::for_each_tag_in_dags('tapp', false, &run);
  sys::for_each_tag_in_dags('tool', false, &run);
} catch (...) {
  return 1;
}
