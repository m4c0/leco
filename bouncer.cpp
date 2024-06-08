#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "dag2.hpp"
#include "die.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "sim.hpp"

#include <string.h>
#include <filesystem>

void prep(sim_sb *cmd, const char *tool);

static const char *common_flags;

static void compile(const dag::node *n) {
  log("compiling", n->source());

  sim_sbt cmd{};
  prep(&cmd, "leco-clang.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->source());
  sim_sb_concat(&cmd, common_flags);
  run(cmd.buffer);

  if (0 != strcmp(".cppm", sim_path_extension(n->source())))
    return;

  sim_sbt pcm{};
  sim_sb_copy(&pcm, n->dag());
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

static void bundle(const dag::node *n) {
  sim_sbt cmd{};
  prep(&cmd, "leco-bundler.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->dag());
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

void bounce(const char *path);
static auto compile_with_deps(const dag::node *n) {
  for (const auto &d : n->build_deps()) {
    bounce(d.c_str());
  }
  return dag::visit_dirty(n, &compile);
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
  dag_read(dag.buffer, [](auto id, auto file) {
    switch (id) {
    case 'tapp':
    case 'tdll':
    case 'tool':
    case 'tmmd':
    }
  });

  auto n = dag::process(path);
  switch (n->root_type()) {
    uint64_t mtime;
  case dag::root_t::none:
    return;
  case dag::root_t::main_mod:
    compile_with_deps(n);
    return;
  case dag::root_t::dll:
    mtime = compile_with_deps(n);
    link(n->dag(), cur_ctx().dll_ext.c_str(), mtime);
    break;
  case dag::root_t::tool:
    if (!cur_ctx().native_target)
      return;

    mtime = compile_with_deps(n);
    link(n->dag(), "exe", mtime);
    break;
  case dag::root_t::app:
    mtime = compile_with_deps(n);
    link(n->dag(), "exe", mtime);
    bundle(n);
    break;
  }
}
