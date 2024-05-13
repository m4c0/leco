#include "context.hpp"
#include "dag.hpp"
#include "host_target.hpp"
#include "sim.h"

bool actool(const char *path) { return false; }
void gen_iphone_plists(const char *exe_path, const char *name) {}
bool find_android_llvm(sim_sb *out) { return false; }

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

bool bundle(const dag::node *n, const char *exe_path) { return true; }
void copy_build_deps(const dag::node *n) {}
