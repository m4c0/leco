#include "../mtime/mtime.h"
#include "cl.hpp"
#include "dag2.hpp"
#include "die.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "sim.hpp"

#include <filesystem>
#include <map>
#include <string.h>
#include <string>

void prep(sim_sb *cmd, const char *tool);

static const char *common_flags;
static const char *target;

static void compile(const char *src, const char *dag) {
  log("compiling", src);

  sim_sbt cmd{};
  prep(&cmd, "leco-clang.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, src);
  sim_sb_concat(&cmd, common_flags);
  run(cmd.buffer);

  if (0 != strcmp(".cppm", sim_path_extension(src)))
    return;

  sim_sbt pcm{};
  sim_sb_copy(&pcm, dag);
  sim_sb_path_set_extension(&pcm, "pcm");

  prep(&cmd, "leco-clang.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, pcm.buffer);
  sim_sb_concat(&cmd, common_flags);
  run(cmd.buffer);
}

static void link(const char *dag, const char *exe, uint64_t mtime) {
  if (mtime <= mtime_of(exe))
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

static void dagger(const char *src, const char *dag) {
  if (mtime_of(src) < mtime_of(dag))
    return;

  sim_sbt args{10240};
  prep(&args, "leco-dagger.exe");
  sim_sb_concat(&args, " -t ");
  sim_sb_concat(&args, target);
  sim_sb_concat(&args, " -i ");
  sim_sb_concat(&args, src);
  sim_sb_concat(&args, " -o ");
  sim_sb_concat(&args, dag);
  run(args.buffer);
}

static auto max(auto a, auto b) { return a > b ? a : b; }

static std::map<std::string, uint64_t> cached{};
static auto build_dag(const char *src) {
  auto &mtime = cached[src];
  if (mtime != 0)
    return mtime;

  mtime = mtime_of(src);

  sim_sbt dag{};
  in2out(src, &dag, "dag", target);
  sim_sbt out{};
  in2out(src, &out, "o", target);

  dagger(src, dag.buffer);
  dag_read(dag.buffer, [&](auto id, auto file) {
    switch (id) {
    case 'head':
      mtime = max(mtime, mtime_of(file));
      break;
    case 'bdep':
    case 'mdep':
      mtime = max(mtime, build_dag(file));
      break;
    default:
      break;
    }
  });
  if (mtime > mtime_of(out.buffer)) {
    compile(src, dag.buffer);
  }

  dag_read(dag.buffer, [&](auto id, auto file) {
    switch (id) {
    case 'impl':
      mtime = max(mtime, build_dag(file));
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

  return build_dag(src);
}
static auto compile_and_link(const char *src, const char *dag,
                             const char *exe) {
  auto mtime = compile_with_deps(src, dag);
  link(dag, exe, mtime);
}

static void bounce(const char *path) {
  cached.clear();

  sim_sbt src{};
  sim_sb_path_copy_real(&src, path);

  sim_sbt dag{};
  in2out(path, &dag, "dag", target);

  dagger(src.buffer, dag.buffer);
  dag_read(dag.buffer, [&](auto id, auto file) {
    switch (id) {
    case 'tapp':
      compile_and_link(path, dag.buffer, file);
      bundle(dag.buffer);
      break;
    case 'tdll':
    case 'tool':
      compile_and_link(path, dag.buffer, file);
      break;
    case 'tmmd':
      compile_with_deps(path, dag.buffer);
      break;
    default:
      break;
    }
  });
}
void bounce(const char *path, const char *target) {
  sim_sbt flags{};
  if (enable_debug_syms())
    sim_sb_concat(&flags, " -g");
  if (is_optimised())
    sim_sb_concat(&flags, " -O");
  common_flags = flags.buffer;

  ::target = target;

  sim_sbt rpath{};
  sim_sb_path_copy_real(&rpath, path);
  bounce(rpath.buffer);
}
