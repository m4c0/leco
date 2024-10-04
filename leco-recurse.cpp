#pragma leco tool
#include "sim.h"

#include <stdint.h>
#include <string.h>

import gopt;
import mtime;
import pprent;
import sim;
import sys;

static const char *common_flags;
static const char *target;

static void link(const char *dag, const char *exe) {
  sys::tool_run("link", "-i %s -o %s %s", dag, exe, common_flags);
}

static void bundle(const char *dag) {
  sys::tool_run("bundler", "-i %s", dag);
}

#if 0 && _WIN32
static void build_rc(const char *path) {
  sim_sbt rc{};
  sim_sb_copy(&rc, path);
  sim_sb_path_set_extension(&rc, "rc");
  if (mtime::of(rc.buffer) == 0) return;

  sim_sbt res{};
  in2out(rc.buffer, &res, "res", target);

  sim_sbt cmd{10240};
  sim_sb_printf(&cmd, "llvm-rc.exe /FO %s %s", res.buffer, rc.buffer);
  sys::run(cmd.buffer);
}
#endif

static void compile(const char * dag) {
  sys::tool_run("pcm", "-i %s", dag);
  sys::tool_run("obj", "-i %s", dag);
}

static void bounce(const char * src);
static void build_bdeps(const char * dag) {
  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'bdag': bounce(file); break;
      default: break;
    }
  });
}

static void bounce(const char * dag) {
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
    case 'tapp':
      compile(dag);
      build_bdeps(dag);
      // build_rc(src);
      link(dag, file);
      bundle(dag);
      break;
    case 'tdll':
    case 'tool':
      compile(dag);
      link(dag, file);
      break;
    case 'tmmd': 
      compile(dag);
      break;
    default: break;
    }
  });
}

static void usage() { sys::die("invalid usage"); }

int main(int argc, char **argv) try {
  sim::sb flags{};
  const char * input {};

  auto opts = gopt_parse(argc, argv, "t:i:gO", [&](auto ch, auto val) {
    switch (ch) {
    case 'i': input = val; break;
    case 't': target = val; break;
    case 'g': flags += " -g"; break;
    case 'O': flags += " -O"; break;
    default: usage(); break;
    }
  });
  if (opts.argc != 0) usage();
  if (!input && !target) usage();

  common_flags = *flags;

  if (input) {
    bounce(input);
    return 0;
  }

  auto path = ("out"_real /= target);
  for (auto file : pprent::list(*path)) {
    auto ext = sim_path_extension(file);
    if (!ext || 0 != strcmp(ext, ".dag")) continue;

    path /= file;
    bounce(*path);
    sim_sb_path_parent(&path);
  }
} catch (...) {
  return 1;
}
