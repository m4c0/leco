#pragma once
#include "sim.hpp"

#include <set>
#include <stdint.h>
#include <string>

namespace dag {
enum class root_t {
  none,
  main_mod,
  dll,
  tool,
  app,
};

class node {
  sim_sbt m_source{};
  sim_sbt m_target{};
  sim_sbt m_dag{};
  sim_sbt m_module_name{};
  sim_sbt m_module_pcm{};
  std::set<std::string> m_build_deps{};
  std::set<std::string> m_headers{};
  std::set<std::string> m_mod_deps{};
  std::set<std::string> m_mod_impls{};
  bool m_compiled{};
  root_t m_root{};
  bool m_recursed{};

public:
  explicit node(const char *n);

  void read_from_cache_file();

  void add_build_dep(const char *src) { m_build_deps.insert(src); }
  void add_header(const char *fname) { m_headers.insert(fname); }
  void add_mod_dep(const char *mod_name) { m_mod_deps.insert(mod_name); }
  void add_mod_impl(const char *mod_impl) { m_mod_impls.insert(mod_impl); }

  void set_app() { m_root = root_t::app; }
  void set_compiled() { m_compiled = true; }
  void set_dll() { m_root = root_t::dll; }
  void set_main_mod() {
    if (m_root == root_t::none)
      m_root = root_t::main_mod;
  }
  void set_recursed() { m_recursed = true; }
  void set_tool() { m_root = root_t::tool; }

  [[nodiscard]] constexpr const auto &build_deps() const noexcept {
    return m_build_deps;
  }
  [[nodiscard]] constexpr const auto &headers() const noexcept {
    return m_headers;
  }
  [[nodiscard]] constexpr const auto &mod_deps() const noexcept {
    return m_mod_deps;
  }
  [[nodiscard]] constexpr const auto &mod_impls() const noexcept {
    return m_mod_impls;
  }

  [[nodiscard]] constexpr auto root_type() const noexcept { return m_root; }

  [[nodiscard]] constexpr bool app() const noexcept {
    return m_root == root_t::app;
  }
  [[nodiscard]] constexpr bool compiled() const noexcept { return m_compiled; }
  [[nodiscard]] constexpr bool dll() const noexcept {
    return m_root == root_t::dll;
  }
  [[nodiscard]] constexpr bool recursed() const noexcept { return m_recursed; }
  [[nodiscard]] constexpr bool root() const noexcept {
    return m_root != root_t::none;
  }
  [[nodiscard]] constexpr bool tool() const noexcept {
    return m_root == root_t::tool;
  }

  [[nodiscard]] constexpr const sim_sb *source_sb() const noexcept {
    return &m_source;
  }
  [[nodiscard]] constexpr const char *source() const noexcept {
    return m_source.buffer;
  }
  [[nodiscard]] constexpr const char *target() const noexcept {
    return m_target.buffer;
  }
  [[nodiscard]] constexpr const char *dag() const noexcept {
    return m_dag.buffer;
  }
  [[nodiscard]] constexpr const char *module_name() const noexcept {
    return m_module_name.buffer;
  }
  [[nodiscard]] constexpr const char *module_pcm() const noexcept {
    return m_module_pcm.buffer;
  }
};

const node *get_node(const char *source);
const node *process(const char *path);
void clear_cache();

uint64_t visit_dirty(const node *n, void *ptr, void (*fn)(void *, const node *));
uint64_t visit_dirty(const node *n, auto &&fn) {
  return visit_dirty(n, &fn, [](void *p, const node *nn) {
    auto pfn = static_cast<decltype(&fn)>(p);
    (*pfn)(nn);
  });
}
} // namespace dag
