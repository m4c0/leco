#include "dag.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "phase2.hpp"

#include <map>

static inline bool path_exists(const char *path) { return mtime_of(path) > 0; }
[[nodiscard]] static bool add_real_abs(std::set<std::string> &set,
                                       const char *path) {
  if (!path_exists(path))
    return false;

  set.insert(path);
  return true;
}

static void infer_module_name(sim_sb *module_name, const sim_sb *src) {
  sim_sb_path_copy_sb_stem(module_name, src);

  auto p = strchr(module_name->buffer, '-');
  if (p != nullptr)
    *p = ':';
}

dag::node::node(const char *n) {
  sim_sb_path_copy_real(&m_source, n);

  in2out(&m_source, &m_target, "o");
  in2out(&m_source, &m_dag, "dag");
  in2out(&m_source, &m_module_pcm, "pcm");
  infer_module_name(&m_module_name, &m_source);
}

bool dag::node::add_build_dep(const char *dep) {
  return add_real_abs(m_build_deps, dep);
}
bool dag::node::add_executable(const char *executable) {
  return add_real_abs(m_executables, executable);
}
bool dag::node::add_header(const char *fname) {
  return add_real_abs(m_headers, fname);
}
bool dag::node::add_library_dir(const char *dir) {
  return add_real_abs(m_library_dirs, dir);
}
bool dag::node::add_mod_dep(const char *mod_name) {
  return add_real_abs(m_mod_deps, mod_name);
}
bool dag::node::add_mod_impl(const char *mod_impl) {
  return add_real_abs(m_mod_impls, mod_impl);
}
bool dag::node::add_resource(const char *resource) {
  return add_real_abs(m_resources, resource);
}
bool dag::node::add_shader(const char *shader) {
  return add_real_abs(m_shaders, shader);
}

static std::map<std::string, dag::node> cache{};
void dag::clear_cache() { cache.clear(); }

static bool compile(dag::node *n) {
  clean(n);

  if (mtime_of(n->source()) > mtime_of(n->dag())) {
    xlog("processing", n->source());
    return dag::execute(n);
  }

  n->read_from_cache_file();
  return true;
}

static auto find(const char *path) {
  dag::node n{path};

  auto [it, inserted] = cache.try_emplace(n.source(), n.source());
  auto *ptr = &(*it).second;

  struct res {
    dag::node *n;
    bool i;
  };
  return res{ptr, inserted};
}

static bool recurse(dag::node *n) {
  for (auto &dep : n->build_deps()) {
    auto [d, ins] = find(dep.c_str());

    if (!d)
      return elog(n->source(), "internal failure");
    if (d->recursed())
      continue;
    if (!compile(d))
      return false;
    if (!recurse(d))
      return false;
  }
  for (auto &dep : n->mod_deps()) {
    if (dep == n->source()) {
      fprintf(stderr, "Self-referencing detected - are you using `import "
                      "<mod>:<part>` instead of `import :<part>`?\n");
      return false;
    }

    auto [d, ins] = find(dep.c_str());

    if (!d)
      return elog(n->source(), "interal failure");
    if (d->recursed())
      continue;
    if (!compile(d))
      return false;
    if (!recurse(d))
      return false;
  }

  n->set_recursed();

  for (auto &impl : n->mod_impls()) {
    // TODO: remove once mod_impls is sim
    sim_sbt imp{};
    sim_sb_copy(&imp, impl.c_str());

    auto [d, ins] = find(imp.buffer);

    if (!d)
      return elog(n->source(), "interal failure");
    if (d->recursed())
      continue;

    if (strcmp(sim_sb_path_extension(&imp), ".cpp") == 0) {
      if (!compile(d))
        return false;
      if (!recurse(d))
        return false;
    }
  }
  return true;
}

dag::node *dag::get_node(const char *source) { return &cache.at(source); }
dag::node *dag::process(const char *path) {
  auto [n, ins] = find(path);
  if (!n || !ins)
    return n;
  if (!compile(n))
    return nullptr;
  if (!n->root())
    return n;

  return recurse(n) ? n : nullptr;
}

void dag::visit(const dag::node *n, bool impls, void *ptr,
                void (*fn)(void *, const dag::node *)) {
  std::set<std::string> visited{};

  const auto rec = [&](auto rec, auto *n) {
    if (visited.contains(n->source()))
      return;

    for (auto &d : n->build_deps()) {
      rec(rec, get_node(d.c_str()));
    }
    for (auto &d : n->mod_deps()) {
      rec(rec, get_node(d.c_str()));
    }

    fn(ptr, n);
    visited.insert(n->source());

    if (impls)
      for (auto &d : n->mod_impls()) {
        rec(rec, get_node(d.c_str()));
      }
  };
  rec(rec, n);
}
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

    // TODO: consider headers as well
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

void dag::visit_all(void *ptr, void (*fn)(void *, const dag::node *)) {
  for (const auto &[k, v] : cache) {
    fn(ptr, &v);
  }
}
