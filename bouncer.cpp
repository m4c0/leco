#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "dag2.hpp"
#include "die.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "sim.hpp"

#include <filesystem>
#include <set>
#include <string.h>
#include <string>

void prep(sim_sb *cmd, const char *tool);

static const char *common_flags;

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

static void link(const char *dag, const char *exe_ext, uint64_t mtime) {
  sim_sbt exe_path{};
  sim_sb_copy(&exe_path, dag);
  sim_sb_path_set_extension(&exe_path, exe_ext);

  if (mtime <= mtime_of(exe_path.buffer))
    return;

  log("linking", exe_path.buffer);

  sim_sbt cmd{};
  prep(&cmd, "leco-link.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, exe_path.buffer);
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
  sim_sb_concat(&args, cur_ctx().target.c_str());
  sim_sb_concat(&args, " -i ");
  sim_sb_concat(&args, src);
  sim_sb_concat(&args, " -o ");
  sim_sb_concat(&args, dag);
  run(args.buffer);
}

static std::set<std::string> visited{};
static void build_dag(const char *src) {
  if (!visited.insert(src).second)
    return;

  sim_sbt dag{};
  in2out(src, &dag, "dag", cur_ctx().target.c_str());

  dagger(src, dag.buffer);
  dag_read(dag.buffer, [](auto id, auto file) {
    switch (id) {
    case 'impl':
    case 'mdep':
      build_dag(file);
      break;
    default:
      break;
    }
  });
}

void bounce(const char *path);
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

  visited.clear();
  build_dag(src);

  return dag::visit_dirty(
      src, [](auto n) { return compile(n->source(), n->dag()); });
}
static auto compile_and_link(const char *src, const char *dag,
                             const char *ext) {
  auto mtime = compile_with_deps(src, dag);
  link(dag, ext, mtime);
}

void bounce(const char *path) {
  sim_sbt flags{};
  if (enable_debug_syms())
    sim_sb_concat(&flags, " -g");
  if (is_optimised())
    sim_sb_concat(&flags, " -O");
  common_flags = flags.buffer;

  sim_sbt src{};
  sim_sb_path_copy_real(&src, path);

  sim_sbt dag{};
  in2out(path, &dag, "dag", cur_ctx().target.c_str());

  dagger(src.buffer, dag.buffer);
  dag_read(dag.buffer, [&](auto id, auto file) {
    switch (id) {
    case 'tapp':
      compile_and_link(path, dag.buffer, "exe");
      bundle(dag.buffer);
      break;
    case 'tdll':
      compile_and_link(path, dag.buffer, cur_ctx().dll_ext.c_str());
      break;
    case 'tool':
      if (cur_ctx().native_target)
        compile_and_link(path, dag.buffer, "exe");
      break;
    case 'tmmd':
      compile_with_deps(path, dag.buffer);
      break;
    default:
      break;
    }
  });
}
