#include "cleaner.hpp"

#include "cl.hpp"
#include "dag.hpp"
#include "sim.h"
#include "llvm/Support/FileSystem.h"
#include <set>

using namespace llvm;

static void remove_parent(std::set<std::string> &already_done,
                          const dag::node *n) {
  // TODO: move "parent path" to dag, since it has multiple uses
  sim_sbt path{256};
  sim_sb_path_copy_parent(&path, n->target());

  auto [it, inserted] = already_done.insert(path.buffer);
  if (inserted)
    sys::fs::remove_directories(path.buffer);
}

void clean(const dag::node *n) {
  assert(n->root() && "only roots should be cleaned up");

  std::set<std::string> done{};

  if (should_clean_all()) {
    dag::visit(n, false, [&](auto *n) { remove_parent(done, n); });
  } else if (should_clean_current()) {
    remove_parent(done, n);
  }
}
