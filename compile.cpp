#include "compile.hpp"

#include "dag.hpp"
#include "die.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"

#include <string.h>

extern const char *leco_argv0;
static void create_deplist(const char *dag) {
  sim_sbt cmd{};
  sim_sb_path_copy_parent(&cmd, leco_argv0);
  sim_sb_path_append(&cmd, "leco-deplist.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  run(cmd.buffer);
}
bool compile(const dag::node *n) {
  sim_sbt path{};
  sim_sb_path_copy_parent(&path, n->target());
  mkdirs(path.buffer);

  vlog("compiling", n->source());
  create_deplist(n->dag());

  sim_sbt cmd{};
  sim_sb_path_copy_parent(&cmd, leco_argv0);
  sim_sb_path_append(&cmd, "leco-clang.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->source());
  if (enable_debug_syms()) {
    sim_sb_concat(&cmd, " -g");
  }
  if (is_optimised()) {
    sim_sb_concat(&cmd, " -O");
  }
  run(cmd.buffer);

  return true;
}
