#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "die.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"

#include <string.h>
#include <filesystem>

void prep(sim_sb *cmd, const char *tool);

static void add_common_flags(sim_sb *cmd) {
  if (enable_debug_syms()) {
    sim_sb_concat(cmd, " -g");
  }
  if (is_optimised()) {
    sim_sb_concat(cmd, " -O");
  }
}

static void compile(const dag::node *n) {
  sim_sbt path{};
  sim_sb_path_copy_parent(&path, n->target());
  mkdirs(path.buffer);

  log("compiling", n->source());

  sim_sbt cmd{};
  prep(&cmd, "leco-clang.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->source());
  add_common_flags(&cmd);
  run(cmd.buffer);

  if (0 != strcmp(".cppm", sim_path_extension(n->source())))
    return;

  prep(&cmd, "leco-clang.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->module_pcm());
  add_common_flags(&cmd);
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
  add_common_flags(&cmd);
  run(cmd.buffer);
}

static void bundle(const dag::node *n) {
  sim_sbt cmd{};
  prep(&cmd, "leco-bundler.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->dag());
  run(cmd.buffer);
}

void bounce(const char *path);
static auto compile_with_deps(const dag::node *n) {
  for (const auto &d : n->build_deps()) {
    bounce(d.c_str());
  }
  return dag::visit_dirty(n, &compile);
}

void bounce(const char *path) {
  auto ext = sim_path_extension(path);
  if (ext == nullptr)
    return;

  if (strcmp(ext, ".cppm") != 0 && strcmp(ext, ".cpp") != 0)
    return;

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
