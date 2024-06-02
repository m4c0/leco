#include "context.hpp"
#include "sim.h"
#include "targets.hpp"

bool find_android_llvm(sim_sb *out) { return false; }

bool should_clean_current() { return false; }
bool should_clean_all() { return false; }
bool is_verbose() { return true; }
bool is_extra_verbose() { return false; }
bool is_optimised() { return true; }
bool enable_debug_syms() { return false; }
bool parse_args(int argc, char **argv) { return true; }

context &cur_ctx() {
  static context i{
      .target = HOST_TARGET,
      .native_target = true,
  };
  return i;
}
bool for_each_target(bool (*fn)()) {
  return fn();
}
