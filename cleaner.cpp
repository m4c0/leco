#include "cleaner.hpp"

#include "cl.hpp"
#include "dag.hpp"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <set>

using namespace llvm;

static void remove_parent(std::set<std::string> &already_done, StringRef tgt) {
  auto path = sys::path::parent_path(tgt);
  auto [it, inserted] = already_done.insert(path.str());
  if (inserted)
    sys::fs::remove_directories(path);
}

void clean(const dag::node *n) {
  assert(n->root() && "only roots should be cleaned up");

  std::set<std::string> done{};

  if (should_clean_all()) {
    dag::visit(n, [&](auto *n) { remove_parent(done, n->target()); });
  } else if (should_clean_current()) {
    remove_parent(done, n->target());
  }
}
