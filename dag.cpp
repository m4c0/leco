#include "dag.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "die.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "phase2.hpp"

#include <map>

static void infer_module_name(sim_sb *module_name, const sim_sb *src) {
  sim_sb_path_copy_sb_stem(module_name, src);

  auto p = strchr(module_name->buffer, '-');
  if (p != nullptr)
    *p = ':';
}

dag::node::node(const char *n) {
  sim_sb_path_copy_real(&m_source, n);

  in2out(source(), &m_target, "o", cur_ctx().target.c_str());
  in2out(source(), &m_dag, "dag", cur_ctx().target.c_str());
  in2out(source(), &m_module_pcm, "pcm", cur_ctx().target.c_str());
  infer_module_name(&m_module_name, &m_source);
}

static std::map<std::string, dag::node> cache{};
void dag::clear_cache() { cache.clear(); }

static dag::node *recurse(const char *path, bool only_roots = false) {
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

  if (mtime_of(n->source()) > mtime_of(n->dag())) {
    if (is_extra_verbose())
      log("processing", n->source());
    n->create_cache_file();
  }
  n->read_from_cache_file();

  auto ext = sim_path_extension(path);
  if (0 != strcmp(".cpp", ext) && 0 != strcmp(".cppm", ext))
    return n;

  if (only_roots && !n->root())
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
const dag::node *dag::process(const char *path) { return recurse(path, true); }

uint64_t dag::visit_dirty(const dag::node *n, void *ptr,
                          void (*fn)(void *, const dag::node *)) {
  std::map<std::string, uint64_t> visited{};

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

    if (mtime > mtime_of(n->target())) {
      fn(ptr, n);
      // Get the mtime again after `fn` possibly changed it
      mtime = mtime_of(n->target());
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
