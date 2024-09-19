#pragma leco tool
#include "in2out.hpp"
#include "sim.hpp"

#include <stdint.h>

import gopt;
import mtime;
import sys;

static const char *common_flags;
static const char *target;
static const char *argv0;

void prep(sim_sb *cmd, const char *tool) {
  sim_sb_path_copy_parent(cmd, argv0);
  sim_sb_path_append(cmd, tool);
}

static void sawblade(const char * src) {
  sim_sbt cmd {};
  prep(&cmd, "leco-sawblade.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, src);
  sim_sb_concat(&cmd, " -t ");
  sim_sb_concat(&cmd, target);
  sys::run(cmd.buffer);
}
static void comp(const char * tool, const char * dag) {
  sim_sbt cmd {};
  prep(&cmd, tool);
  sim_sb_printf(&cmd, " -i %s", dag);
  sim_sb_concat(&cmd, common_flags);
  sys::run(cmd.buffer);
}

static void link(const char *dag, const char *exe) {
  sim_sbt cmd{};
  prep(&cmd, "leco-link.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, exe);
  sim_sb_concat(&cmd, common_flags);
  sys::run(cmd.buffer);
}

static void bundle(const char *dag) {
  sim_sbt cmd{};
  prep(&cmd, "leco-bundler.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  sys::run(cmd.buffer);
}

static void build_rc(const char *path) {
#if _WIN32
  sim_sbt rc{};
  sim_sb_copy(&rc, path);
  sim_sb_path_set_extension(&rc, "rc");
  if (mtime::of(rc.buffer) == 0) return;

  sim_sbt res{};
  in2out(rc.buffer, &res, "res", target);

  sim_sbt cmd{10240};
  sim_sb_printf(&cmd, "llvm-rc.exe /FO %s %s", res.buffer, rc.buffer);
  sys::run(cmd.buffer);
#endif
}

static void compile(const char * dag) {
  comp("leco-pcm.exe", dag);
  comp("leco-obj.exe", dag);
}

static void bounce(const char * src);
static void build_bdeps(const char * dag) {
  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'bdep': bounce(file); break;
      default: break;
    }
  });
}

static void bounce(const char * src) {
  sim_sbt dag{};
  in2out(src, &dag, "dag", target);

  sawblade(src);

  sys::dag_read(dag.buffer, [&](auto id, auto file) {
    switch (id) {
    case 'tapp':
      compile(dag.buffer);
      build_bdeps(dag.buffer);
      build_rc(src);
      link(dag.buffer, file);
      bundle(dag.buffer);
      break;
    case 'tdll':
    case 'tool':
      compile(dag.buffer);
      link(dag.buffer, file);
      break;
    case 'tmmd': 
      compile(dag.buffer);
      break;
    default: break;
    }
  });
}

static void usage() { sys::die("invalid usage"); }

int main(int argc, char **argv) try {
  sim_sbt flags{};
  sim_sbt rpath{};

  auto opts = gopt_parse(argc, argv, "t:i:gO", [&](auto ch, auto val) {
    switch (ch) {
    case 'i': sim_sb_path_copy_real(&rpath, val); break;
    case 't': target = val; break;
    case 'g': sim_sb_concat(&flags, " -g"); break;
    case 'O': sim_sb_concat(&flags, " -O"); break;
    default: usage(); break;
    }
  });
  if (opts.argc != 0 || rpath.len == 0) usage();

  argv0 = argv[0];
  common_flags = flags.buffer;
  bounce(rpath.buffer);
} catch (...) {
  return 1;
}
