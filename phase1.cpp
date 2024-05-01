#include "context.hpp"
#include "dag.hpp"
#include "host_target.hpp"
#include "sim.h"

bool actool(const char *path) { return false; }
void gen_iphone_plists(const char *exe_path, const char *name) {}
bool find_android_llvm(sim_sb *out) { return false; }

void clean(const dag::node *) {}

bool dag::node::is_cache_file_fresh() const { return false; }
void dag::node::write_to_cache_file() const {}
bool dag::node::read_from_cache_file() {
  fprintf(stderr, "Error: attempt of reading cache file in Phase 1\n");
  return false;
}

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
