#include "dag.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "die.hpp"
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

  in2out(source(), &m_target, "o", cur_ctx().target.c_str());
  in2out(source(), &m_dag, "dag", cur_ctx().target.c_str());
  in2out(source(), &m_module_pcm, "pcm", cur_ctx().target.c_str());
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

void dag::visit_all(void *ptr, void (*fn)(void *, const dag::node *)) {
  for (const auto &[k, v] : cache) {
    fn(ptr, &v);
  }
}
