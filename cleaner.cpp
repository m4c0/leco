#include "cleaner.hpp"

#include "cl.hpp"
#include "dag.hpp"
#include "log.hpp"
#include "sim.hpp"

#include "../minirent/minirent.h"

#include <assert.h>
#include <set>

#ifdef _WIN32
#define unlink _unlink
#else
#include <unistd.h>
#endif

static void remove_recursive(sim_sb *path) {
  DIR *dir = opendir(path->buffer);
  if (dir != nullptr) {
    dirent *ds;
    while ((ds = readdir(dir)) != nullptr) {
      if (ds->d_name[0] == '.')
        continue;
      sim_sb_path_append(path, ds->d_name);
      remove_recursive(path);
      sim_sb_path_parent(path);
    }
    closedir(dir);
  }
  vlog("removing", path->buffer);
  unlink(path->buffer);
}
static void remove_parent(std::set<std::string> &already_done,
                          const dag::node *n) {
  sim_sbt path{};
  sim_sb_path_copy_parent(&path, n->target());

  auto [it, inserted] = already_done.insert(path.buffer);
  if (!inserted)
    return;

  return remove_recursive(&path);
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
