#include "cleaner.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "sim.h"

bool actool(const char *path) { return false; }
void gen_iphone_plists(const char *exe_path, const char *name) {}
bool find_android_llvm(sim_sb *out) { return false; }

void clean(const dag::node *) {}

void dag::node::write_to_cache_file() const {}
bool dag::node::read_from_cache_file() { return false; }

bool is_verbose() { return true; }
bool is_extra_verbose() { return false; }
bool is_optimised() { return true; }
bool enable_debug_syms() { return false; }
bool parse_args(int argc, char **argv) { return true; }

context &cur_ctx() {
  static context i {
#if _WIN32
    .target = "x86_64-pc-windows-msvc",
#elif __APPLE__
    .target = "x86_64-apple-macosx11.6.0",
#else
    .target = "x86_64-pc-linux-gnu",
#endif
    .native_target = true,
  };
  return i;
}
bool for_each_target(bool (*fn)()) {
  return fn();
}
