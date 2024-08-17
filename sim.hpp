#pragma once
#include "sim.h"

#ifdef __linux__
#include <linux/limits.h>
#elif _WIN32
#include <stdlib.h>
#define PATH_MAX _MAX_PATH
#else
#include <limits.h>
#endif

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
