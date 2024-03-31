#pragma once
#include "context.hpp"
#include "sim.h"

static void in2out(const sim_sb *in, sim_sb *out, const char *ext) {
  sim_sb_copy(out, in->buffer);
  if (strcmp(in->buffer, SIM_PATHSEP_S "out" SIM_PATHSEP_S) != 0) {
    sim_sb_path_parent(out);
    sim_sb_path_append(out, "out");
    sim_sb_path_append(out, cur_ctx().target.c_str());
    sim_sb_path_append(out, sim_sb_path_filename(in));
  }
  sim_sb_path_set_extension(out, ext);
}
