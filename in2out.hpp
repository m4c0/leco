#pragma once
#include "sim.hpp"

#include <string.h>

static void in2out(const char *in, sim_sb *out, const char *ext,
                   const char *target) {
  sim_sb_copy(out, in);
  if (strstr(in, SIM_PATHSEP_S "out" SIM_PATHSEP_S) == nullptr) {
    sim_sb_path_parent(out);
    sim_sb_path_append(out, "out");
    sim_sb_path_append(out, target);
    sim_sb_path_append(out, sim_path_filename(in));
  }
  sim_sb_path_set_extension(out, ext);
}
