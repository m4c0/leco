#pragma once
#include "pathmax.h"
#include "sim.h"

// TODO: modularise
struct sim_sbt : sim_sb {
  sim_sbt() { sim_sb_new(this, PATH_MAX); }
  sim_sbt(unsigned sz) { sim_sb_new(this, sz); }
  ~sim_sbt() { sim_sb_delete(this); }

  sim_sbt(const sim_sbt &) = delete;
  sim_sbt(sim_sbt &&) = delete;
  sim_sbt &operator=(const sim_sbt &) = delete;
  sim_sbt &operator=(sim_sbt &&) = delete;
};
