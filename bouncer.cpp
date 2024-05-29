#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "die.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "phase2.hpp"
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

  vlog("compiling", n->source());

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

static void link(const dag::node *n, const char *exe) {
  vlog("linking", exe);

  sim_sbt cmd{};
  prep(&cmd, "leco-link.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->dag());
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, exe);
  add_common_flags(&cmd);
  run(cmd.buffer);
}

void bounce(const char *path) {
  auto ext = sim_path_extension(path);
  if (ext == nullptr)
    return;

  if (strcmp(ext, ".cppm") != 0 && strcmp(ext, ".cpp") != 0)
    return;

  auto n = dag::process(path);
  if (!n->root())
    return;

  if (n->tool() && !cur_ctx().native_target)
    return;

  for (const auto &d : n->build_deps()) {
    bounce(d.c_str());
  }

  auto mtime = dag::visit_dirty(n, &compile);

  if (!n->app() && !n->tool() && !n->dll())
    return;

  const char *exe_ext = n->dll() ? cur_ctx().dll_ext.c_str() : "exe";

  sim_sbt exe_path{};
  sim_sb_copy(&exe_path, n->dag());
  sim_sb_path_set_extension(&exe_path, exe_ext);

  if (mtime > mtime_of(exe_path.buffer)) {
    link(n, exe_path.buffer);
  }

  if (n->app()) {
    bundle(n->dag());
  }
}
