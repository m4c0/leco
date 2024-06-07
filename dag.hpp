#pragma once
#include "sim.hpp"

#include <set>
#include <stdint.h>
#include <string>

namespace dag {
enum class root_t {
  none = 0,
  main_mod = 'tmmd',
  dll = 'tdll',
  tool = 'tool',
  app = 'tapp',
};

class node {
  sim_sbt m_source{};
  sim_sbt m_dag{};
  std::set<std::string> m_build_deps{};
  std::set<std::string> m_headers{};
  std::set<std::string> m_mod_deps{};
  std::set<std::string> m_mod_impls{};
  bool m_compiled{};
  root_t m_root{};
  bool m_recursed{};

public:
  explicit node(const char *n);

  void add_build_dep(const char *src) { m_build_deps.insert(src); }
  void add_header(const char *fname) { m_headers.insert(fname); }
  void add_mod_dep(const char *mod_name) { m_mod_deps.insert(mod_name); }
  void add_mod_impl(const char *mod_impl) { m_mod_impls.insert(mod_impl); }

  void set_compiled() { m_compiled = true; }
  void set_recursed() { m_recursed = true; }

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
  void set_root_type(root_t t) { m_root = t; }

  [[nodiscard]] constexpr bool compiled() const noexcept { return m_compiled; }
  [[nodiscard]] constexpr bool recursed() const noexcept { return m_recursed; }

  [[nodiscard]] constexpr const sim_sb *source_sb() const noexcept {
    return &m_source;
  }
  [[nodiscard]] constexpr const char *source() const noexcept {
    return m_source.buffer;
  }
  [[nodiscard]] constexpr const char *dag() const noexcept {
    return m_dag.buffer;
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
