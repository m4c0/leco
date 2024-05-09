#include "compile.hpp"

#include "dag.hpp"
#include "die.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"

extern const char *leco_argv0;
bool compile(const dag::node *n) {
  sim_sbt path{};
  sim_sb_path_copy_parent(&path, n->target());
  mkdirs(path.buffer);

  vlog("compiling", n->source());

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
