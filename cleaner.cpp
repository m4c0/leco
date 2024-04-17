#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "log.hpp"
#include "phase2.hpp"
#include "sim.hpp"

#include "../minirent/minirent.h"

#include <assert.h>
#include <string.h>
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

static std::set<std::string> done{};

void clean(const dag::node *n) {
  if (!should_clean_current())
    return;

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, n->target());

  auto [it, inserted] = done.insert(path.buffer);
  if (!inserted)
    return;

  if (should_clean_all()) {
    remove_recursive(&path);
    return;
  }

  sim_sbt cwd{};
  sim_sb_path_copy_real(&cwd, "out");
  sim_sb_path_append(&cwd, cur_ctx().target.c_str());
  if (0 == strcmp(cwd.buffer, path.buffer))
    remove_recursive(&path);
}
