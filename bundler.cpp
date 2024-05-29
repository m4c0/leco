#include "../mtime/mtime.h"
#include "context.hpp"
#include "dag.hpp"
#include "die.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"

#include <filesystem>

void prep(sim_sb *cmd, const char *tool);

static void copy(const char *with, const char *dag, const char *to) {
  sim_sbt cmd{};
  prep(&cmd, with);
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, dag);
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, to);
  run(cmd.buffer);
}

void bundle(const char *dag) {
  sim_sbt exe{};
  sim_sb_copy(&exe, dag);
  sim_sb_path_set_extension(&exe, "exe");

  sim_sbt stem{};
  sim_sb_path_copy_sb_stem(&stem, &exe);

  sim_sbt exe_path{};
  sim_sb_copy(&exe_path, exe.buffer);
  cur_ctx().app_exe_path(&exe_path, stem.buffer);
  sim_sb_path_parent(&exe_path);
  mkdirs(exe_path.buffer);

  sim_sbt res_path{};
  sim_sb_copy(&res_path, exe_path.buffer);
  sim_sb_path_append(&res_path, "hack"); // TODO: pull logic from target_defs
  cur_ctx().app_res_path(&res_path);
  mkdirs(res_path.buffer);

  copy("leco-exs.exe", dag, exe_path.buffer);
  copy("leco-rsrc.exe", dag, res_path.buffer);

  cur_ctx().bundle(exe.buffer, stem.buffer);
}
