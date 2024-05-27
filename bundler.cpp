#include "../mtime/mtime.h"
#include "context.hpp"
#include "dag.hpp"
#include "die.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"

#include <filesystem>

void prep(sim_sb *cmd, const char *tool);

static void in2exe(const dag::node *n, sim_sb *out) {
  in2out(n->source(), out, "exe", cur_ctx().target.c_str());

  if (n->app()) {
    sim_sbt stem{};
    sim_sb_path_copy_sb_stem(&stem, out);

    cur_ctx().app_exe_path(out, stem.buffer);

    sim_sbt path{};
    sim_sb_path_copy_parent(&path, out->buffer);
    mkdirs(path.buffer);
  }
}

static void copy(const char *with, const dag::node *n, const char *to) {
  sim_sbt cmd{};
  prep(&cmd, with);
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->dag());
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, to);
  run(cmd.buffer);
}

void bundle(const dag::node *n) {
  sim_sbt exe{};
  sim_sb_copy(&exe, n->dag());
  sim_sb_path_set_extension(&exe, "exe");

  sim_sbt res_path{};
  sim_sb_copy(&res_path, exe.buffer);
  cur_ctx().app_res_path(&res_path);
  mkdirs(res_path.buffer);

  sim_sbt exe_path{};
  in2exe(n, &exe_path);
  sim_sb_path_parent(&exe_path);

  copy("leco-exs.exe", n, exe_path.buffer);
  copy("leco-rsrc.exe", n, res_path.buffer);

  cur_ctx().bundle(exe.buffer, n->module_name());
}
