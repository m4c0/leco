#include "cl.hpp"
#include "cleaner.hpp"
#include "dag.hpp"

using namespace llvm;

static void remove_parent(StringSet<> &already_done, StringRef tgt) {
  auto path = sys::path::parent_path(tgt);
  if (already_done.contains(path))
    return;

  sys::fs::remove_directories(path);

  already_done.insert(path);
}

void clean(const dag::node *n) {
  assert(n->root() && "only roots should be cleaned up");

  StringSet<> done{};

  if (should_clean_current()) {
    remove_parent(done, n->target());
    return;
  }
  if (should_clean_all()) {
    dag::visit(n, [&](auto *n) { remove_parent(done, n->target()); });
    return;
  }
}
