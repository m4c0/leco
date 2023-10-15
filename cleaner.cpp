#include "cl.hpp"
#include "cleaner.hpp"
#include "dag.hpp"
#include "in2out.hpp"

using namespace llvm;

static void remove_parent(StringSet<> &already_done, StringRef src) {
  auto src_path = sys::path::parent_path(src);
  if (already_done.contains(src_path))
    return;

  SmallString<256> out{};
  in2out(src, out);

  auto path = sys::path::parent_path(out);
  sys::fs::remove_directories(path);

  already_done.insert(src_path);
}

void clean(const dag::node *n) {
  assert(n->root() && "only roots should be cleaned up");

  StringSet<> done{};

  if (should_clean_current()) {
    remove_parent(done, n->source());
    return;
  }
  if (should_clean_all()) {
    dag::visit(n, [&](auto *n) { remove_parent(done, n->source()); });
    return;
  }
}
