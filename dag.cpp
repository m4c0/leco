#include "dag.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "in2out.hpp"
#include "llvm/Support/FileSystem.h"
#include <fstream>

using namespace llvm;

static inline bool path_exists(const char *path) { return mtime_of(path) > 0; }
static void real_abs(SmallVectorImpl<char> &buf, StringRef path) {
  sys::fs::real_path(path, buf);
  sys::fs::make_absolute(buf);
}
[[nodiscard]] static bool add_real_abs(StringSet<> &set, StringRef path) {
  if (!path_exists(path.str().c_str()))
    return false;

  SmallString<256> abs{};
  sys::fs::real_path(path, abs);
  sys::fs::make_absolute(abs);

  set.insert(abs);
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

bool dag::node::add_executable(const char *executable) {
  return add_real_abs(m_executables, executable);
}
bool dag::node::add_include_dir(const char *dir) {
  return add_real_abs(m_include_dirs, dir);
}
bool dag::node::add_library_dir(const char *dir) {
  return add_real_abs(m_library_dirs, dir);
}
bool dag::node::add_mod_dep(const char *mod_name) {
  sim_sbt pp{256};
  sim_sb_copy(&pp, mod_name);

  // Module parts
  auto p = strchr(pp.buffer, ':');
  if (p != nullptr) {
    *p = '-';

    sim_sbt dep{256};
    sim_sb_path_copy_parent(&dep, source());
    sim_sb_path_append(&dep, pp.buffer);
    sim_sb_concat(&dep, ".cppm");
    if (add_real_abs(m_mod_deps, dep.buffer))
      return true;
  }

  // Module in the same folder
  sim_sbt dep{256};
  sim_sb_path_copy_parent(&dep, source());
  sim_sb_path_append(&dep, mod_name);
  sim_sb_concat(&dep, ".cppm");
  if (add_real_abs(m_mod_deps, dep.buffer))
    return true;

  // Module in sibling folder
  sim_sb_path_parent(&dep);
  sim_sb_path_parent(&dep);
  sim_sb_path_append(&dep, mod_name);
  sim_sb_path_append(&dep, mod_name);
  sim_sb_concat(&dep, ".cppm");
  if (add_real_abs(m_mod_deps, dep.buffer))
    return true;

  // Module in sibling folder with "-" instead of "_"
  while ((p = strchr(pp.buffer, '_')) != nullptr) {
    *p = '-';
  }
  sim_sb_path_parent(&dep);
  sim_sb_path_parent(&dep);
  sim_sb_path_append(&dep, pp.buffer);
  sim_sb_path_append(&dep, mod_name);
  sim_sb_concat(&dep, ".cppm");
  return add_real_abs(m_mod_deps, dep.buffer);
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

static StringMap<dag::node> cache{};
void dag::clear_cache() { cache.clear(); }

void dag::errlog(const dag::node *n, const char *msg) {
  errs() << msg << " " << n->source() << "\n";
}
void dag::xlog(const dag::node *n, const char *msg) {
  if (!is_extra_verbose())
    return;
  dag::errlog(n, msg);
}

static bool still_fresh(dag::node *n) {
  auto dag_mtime = mtime_of(n->dag());
  return dag_mtime > mtime_of(n->source()) &&
         dag_mtime > mtime_of("../leco/leco.exe");
}

static bool compile(dag::node *n) {
  if (!still_fresh(n)) {
    dag::xlog(n, "dag compilation");
    return dag::execute(n);
  }

  // Capture both success and failures. Failures might leave inconsistencies
  // in the node, which would be worse if we read and store it again
  return n->read_from_cache_file();
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
  for (auto &dep : n->mod_deps()) {
    if (dep.first() == n->source()) {
      errs() << "Self-referencing detected - are you using `import "
                "<mod>:<part>` instead of `import :<part>`?\n";
      return false;
    }

    auto [d, ins] = find(dep.first().str().c_str());

    if (!d)
      return false;
    if (d->recursed())
      continue;
    if (!compile(d))
      return false;
    if (!recurse(d))
      return false;

    d->set_recursed();
  }
  for (auto &impl : n->mod_impls()) {
    // TODO: remove once mod_impls is sim
    sim_sbt imp{256};
    sim_sb_copy(&imp, impl.first().str().c_str());

    auto [d, ins] = find(imp.buffer);

    if (!d)
      return false;
    if (d->recursed())
      continue;

    if (strcmp(sim_sb_path_extension(&imp), ".cpp") == 0) {
      if (!compile(d))
        return false;
      if (!recurse(d))
        return false;
      // TODO: do we care if we have a CPP file which isn't a C++20 impl unit?
      if (!d->add_mod_dep(n->module_name()))
        return false;
    }

    d->set_recursed();
  }
  return true;
}

dag::node *dag::get_node(const char *source) {
  auto it = cache.find(source);
  return it == cache.end() ? nullptr : &(*it).second;
}
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

uint64_t dag::visit_dirty(const node *n, function_ref<void(const node *)> fn) {
  StringMap<uint64_t> visited{};

  uint64_t max{};
  const auto rec = [&](auto rec, auto *n, uint64_t pmt) -> uint64_t {
    auto it = visited.find(n->source());
    if (it != visited.end()) {
      max = max > it->second ? max : it->second;
      return it->second;
    }

    auto mtime = mtime_of(n->source());
    mtime = mtime > pmt ? mtime : pmt;

    for (auto &d : n->mod_deps()) {
      auto dmt = rec(rec, get_node(d.first().str().c_str()), {});
      mtime = mtime > dmt ? mtime : dmt;
    }

    if (mtime > mtime_of(n->target())) {
      fn(n);
      // Get the mtime again after `fn` possibly changed it
      mtime = mtime_of(n->target());
    }

    max = max > mtime ? max : mtime;
    visited[n->source()] = mtime;

    for (auto &d : n->mod_impls()) {
      rec(rec, get_node(d.first().str().c_str()), mtime);
    }

    return mtime;
  };
  rec(rec, n, {});
  return max;
}
