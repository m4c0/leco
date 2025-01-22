#pragma leco tool

#include <stdio.h>
#include <string.h>

import gopt;
import mtime;
import popen;
import pprent;
import sim;
import sys;
import sysstd;

static void usage() {
  sys::die(R"(
Compiles shaders referenced by their modules via pragmas.

Usage: ../leco/leco.exe shaders -i <dag>

Where:
        -i  root DAG to start scanning

Requires DAGs created via leco-dagger.
)"); 
}

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
static void build_shader(const char * dag, const char * file) {
  auto out = sim::path_parent(dag) / sim::path_filename(file) + ".spv";
  if (!must_recompile(file, mtime::of(*out))) return;

  sys::log("compiling shader", file);

  char * args[] {
    sysstd::strdup("glslangValidator"),
    sysstd::strdup("--target-env"),
    sysstd::strdup("spirv1.3"),
    sysstd::strdup("-V"),
    sysstd::strdup("-o"),
    *out,
    sysstd::strdup(file),
    0,
  };
  p::proc p { args };

  while (p.gets()) {
    auto line = p.last_line_read();
    if (0 == strncmp(line, "ERROR: ", 7))  {
      if (line[7] == '/') line += 7;
      else if (line[8] == ':') line += 7;
    }
    sim::sb l { line };
    if (l[l.len - 2] == '\r') {
      l[l.len - 2] = '\n';
      l[l.len - 1] = 0;
    }
    fputs(*l, stdout);
  }

  if (p.wait() != 0) sys::die("shader compilation failed");
}

int main(int argc, char ** argv) try {
  sim::sb input {};

  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = sim::path_real(val); break;
      default: usage();
    }
  });
  if (opts.argc) usage();
  if (!input.len) usage();

  sys::recurse_dag(*input, [&](auto id, auto file) {
    if (id == 'shdr') build_shader(*input, file);
  });
} catch (...) {
  return 1;
}
