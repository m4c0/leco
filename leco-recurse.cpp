#pragma leco tool
#include "dag2.hpp"
#include "in2out.hpp"
#include "sim.hpp"

#include <map>
#include <string.h>
#include <string>

import gopt;
import mtime;
import sys;

static const char *common_flags;
static const char *target;
static const char *argv0;
static bool reformat;

void prep(sim_sb *cmd, const char *tool) {
  sim_sb_path_copy_parent(cmd, argv0);
  sim_sb_path_append(cmd, tool);
}

static void format(const char *cpp) {
  if (!reformat)
    return;

  sim_sbt cmd{};
  prep(&cmd, "leco-format.exe");
  if (mtime::of(cmd.buffer) == 0)
    return;

  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, cpp);
  sys::run(cmd.buffer);
}

static void sawblade(const char * src) {
  sim_sbt cmd {};
  prep(&cmd, "leco-sawblade.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, src);
  sim_sb_concat(&cmd, " -t ");
  sim_sb_concat(&cmd, target);
  run(cmd.buffer);
}

static void compile(const char *src, const char *dag) {
  log("compiling", src);

  sim_sbt cmd{};
  prep(&cmd, "leco-clang.exe");
  sim_sb_printf(&cmd, " -i %s -t %s", src, target);
  sim_sb_concat(&cmd, common_flags);
  run(cmd.buffer);

  format(src);

  if (0 != strcmp(".cppm", sim_path_extension(src)))
    return;

  sim_sbt pcm{};
  sim_sb_copy(&pcm, dag);
  sim_sb_path_set_extension(&pcm, "pcm");

  prep(&cmd, "leco-clang.exe");
  sim_sb_printf(&cmd, " -i %s -t %s", pcm.buffer, target);
  sim_sb_concat(&cmd, common_flags);
  run(cmd.buffer);
}

static void link(const char *dag, const char *exe, uint64_t mtime) {
  if (mtime <= mtime::of(exe))
    return;

  log("linking", exe);

  sim_sbt cmd{};
  prep(&cmd, "leco-link.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, exe);
  sim_sb_concat(&cmd, common_flags);
  run(cmd.buffer);
}

static void bundle(const char *dag) {
  sim_sbt cmd{};
  prep(&cmd, "leco-bundler.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  run(cmd.buffer);
}

struct mtime_pair {
  uint64_t spec{};
  uint64_t impl{};
};
static auto max(uint64_t a, uint64_t b) { return a > b ? a : b; }
static auto max(mtime_pair a, mtime_pair b) {
  return mtime_pair{max(a.spec, b.spec), max(a.impl, b.impl)};
}

static std::map<std::string, mtime_pair> cached{};
static auto build_dag(const char *src) {
  auto &mtime = cached[src];
  if (mtime.spec != 0)
    return mtime;

  mtime.spec = mtime::of(src);

  sim_sbt dag{};
  in2out(src, &dag, "dag", target);
  sim_sbt out{};
  in2out(src, &out, "o", target);

  dag_read(dag.buffer, [&](auto id, auto file) {
    switch (id) {
    case 'head':
      mtime.spec = max(mtime.spec, mtime::of(file));
      break;
    case 'bdep':
    case 'mdep':
      mtime = max(mtime, build_dag(file));
      break;
    default:
      break;
    }
  });
  if (mtime.spec > mtime::of(out.buffer)) {
    compile(src, dag.buffer);
  }
  mtime.impl = max(mtime.impl, mtime::of(out.buffer));

  dag_read(dag.buffer, [&](auto id, auto file) {
    switch (id) {
    case 'impl':
      mtime.impl = max(mtime.impl, build_dag(file).impl);
      break;
    default:
      break;
    }
  });

  return mtime;
}

static void bounce(const char *path);
static auto compile_with_deps(const char *src, const char *dag) {
  dag_read(dag, [](auto id, auto file) {
    switch (id) {
    case 'bdep':
      bounce(file);
      break;
    default:
      break;
    }
  });

  return build_dag(src).impl;
}

static void build_rc(const char *path) {
#if _WIN32
  sim_sbt rc{};
  sim_sb_copy(&rc, path);
  sim_sb_path_set_extension(&rc, "rc");
  if (mtime::of(rc.buffer) == 0)
    return;

  sim_sbt res{};
  in2out(rc.buffer, &res, "res", target);

  sim_sbt cmd{10240};
  sim_sb_printf(&cmd, "llvm-rc.exe /FO %s %s", res.buffer, rc.buffer);
  sys::run(cmd.buffer);
#endif
}

static void bounce(const char *path) {
  cached.clear();

  sim_sbt src{};
  sim_sb_path_copy_real(&src, path);

  sim_sbt dag{};
  in2out(path, &dag, "dag", target);

  sawblade(src.buffer);

  dag_read(dag.buffer, [&](auto id, auto file) {
    switch (id) {
    case 'tapp':
      build_rc(path);
      link(dag.buffer, file, compile_with_deps(path, dag.buffer));
      bundle(dag.buffer);
      break;
    case 'tdll':
    case 'tool':
      link(dag.buffer, file, compile_with_deps(path, dag.buffer));
      break;
    case 'tmmd':
      compile_with_deps(path, dag.buffer);
      break;
    default:
      break;
    }
  });
}

static void usage() { die("invalid usage"); }

int main(int argc, char **argv) try {
  sim_sbt flags{};
  sim_sbt rpath{};

  auto opts = gopt_parse(argc, argv, "ft:i:gO", [&](auto ch, auto val) {
    switch (ch) {
    case 'f':
      reformat = true;
      break;
    case 'i':
      sim_sb_path_copy_real(&rpath, val);
      break;
    case 't':
      target = val;
      break;
    case 'g':
      sim_sb_concat(&flags, " -g");
      break;
    case 'O':
      sim_sb_concat(&flags, " -O");
      break;
    default:
      usage();
    }
  });
  if (opts.argc != 0 || rpath.len == 0)
    usage();

  argv0 = argv[0];
  common_flags = flags.buffer;
  bounce(rpath.buffer);
} catch (...) {
  return 1;
}
