#pragma once
#include "dag.hpp"
#include "in2out.hpp"
#include "mkdir.h"
#include "sim.hpp"

static void in2exe(const dag::node *n, sim_sb *out) {
  std::string ext = n->dll() ? cur_ctx().dll_ext : "exe";
  in2out(n->source(), out, ext.c_str(), cur_ctx().target.c_str());

  if (n->app()) {
    sim_sbt stem{};
    sim_sb_path_copy_sb_stem(&stem, out);

    cur_ctx().app_exe_path(out, stem.buffer);

    sim_sbt path{};
    sim_sb_path_copy_parent(&path, out->buffer);
    mkdirs(path.buffer);
  }
}
