#include "dag.hpp"

#include "../mtime/mtime.h"
#include "context.hpp"
#include "dag2.hpp"
#include "die.hpp"
#include "in2out.hpp"

#include <map>

void prep(sim_sb *cmd, const char *tool);

dag::node::node(const char *n) {
  sim_sb_path_copy_real(&m_source, n);

  in2out(source(), &m_dag, "dag", cur_ctx().target.c_str());
}

static std::map<std::string, dag::node> cache{};
void dag::clear_cache() { cache.clear(); }

static dag::node *recurse(const char *path) {
  sim_sbt real{};
  sim_sb_path_copy_real(&real, path);
  auto it = cache.find(real.buffer);
  if (it == cache.end()) {
    it = cache.emplace(real.buffer, real.buffer).first;
  }

  dag::node *n = &it->second;
  if (!n)
    die("internal failure");
  if (n->recursed())
    return n;

  dag_read(n->dag(), [n](auto id, auto file) {
    switch (id) {
    case 'bdep':
      n->add_build_dep(file);
      break;
    case 'head':
      n->add_header(file);
      break;
    case 'impl':
      n->add_mod_impl(file);
      break;
    case 'mdep':
      n->add_mod_dep(file);
      break;
    }
  });

  auto ext = sim_path_extension(path);
  if (0 != strcmp(".cpp", ext) && 0 != strcmp(".cppm", ext))
    return n;

  for (auto &dep : n->build_deps()) {
    recurse(dep.c_str());
  }
  for (auto &dep : n->mod_deps()) {
    if (dep == n->source())
      die("Self-referencing detected - are you using `import <mod>:<part>` "
          "instead of `import :<part>`?\n");

    recurse(dep.c_str());
  }

  n->set_recursed();

  for (auto &impl : n->mod_impls()) {
    recurse(impl.c_str());
  }

  return n;
}

const dag::node *dag::get_node(const char *source) { return &cache.at(source); }

uint64_t dag::visit_dirty(const char *path, void *ptr,
                          void (*fn)(void *, const dag::node *)) {
  std::map<std::string, uint64_t> visited{};

  auto n = recurse(path);

  uint64_t max{};
  const auto rec = [&](auto rec, auto *n, uint64_t pmt) -> uint64_t {
    auto it = visited.find(n->source());
    if (it != visited.end()) {
      max = max > it->second ? max : it->second;
      return it->second;
    }

    auto mtime = mtime_of(n->source());
    mtime = mtime > pmt ? mtime : pmt;

    for (auto &d : n->headers()) {
      auto dmt = mtime_of(d.c_str());
      mtime = mtime > dmt ? mtime : dmt;
    }
    for (auto &d : n->build_deps()) {
      auto dmt = rec(rec, get_node(d.c_str()), {});
      mtime = mtime > dmt ? mtime : dmt;
    }
    for (auto &d : n->mod_deps()) {
      auto dmt = rec(rec, get_node(d.c_str()), {});
      mtime = mtime > dmt ? mtime : dmt;
    }

    sim_sbt tgt{};
    sim_sb_copy(&tgt, n->dag());
    sim_sb_path_set_extension(&tgt, "o");
    if (mtime > mtime_of(tgt.buffer)) {
      fn(ptr, n);
      // Get the mtime again after `fn` possibly changed it
      mtime = mtime_of(tgt.buffer);
    }

    max = max > mtime ? max : mtime;
    visited[n->source()] = mtime;

    for (auto &d : n->mod_impls()) {
      rec(rec, get_node(d.c_str()), mtime);
    }

    return mtime;
  };
  rec(rec, n, {});
  return max;
}
