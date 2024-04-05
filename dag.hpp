#pragma once
#include "sim.hpp"
#include <set>
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
  std::set<std::string> m_executables{};
  std::set<std::string> m_frameworks{};
  std::set<std::string> m_libraries{};
  std::set<std::string> m_library_dirs{};
  std::set<std::string> m_mod_deps{};
  std::set<std::string> m_mod_impls{};
  std::set<std::string> m_resources{};
  std::set<std::string> m_shaders{};
  bool m_compiled{};
  root_t m_root{};
  bool m_recursed{};

public:
  explicit node(const char *n);

  bool read_from_cache_file();
  void write_to_cache_file() const;

  [[nodiscard]] bool add_executable(const char *e);
  void add_framework(const char *fw) { m_frameworks.insert(fw); }
  void add_library(const char *lib) { m_libraries.insert(lib); }
  [[nodiscard]] bool add_library_dir(const char *dir);
  [[nodiscard]] bool add_mod_dep(const char *mod_name);
  [[nodiscard]] bool add_mod_impl(const char *mod_impl);
  [[nodiscard]] bool add_resource(const char *res);
  [[nodiscard]] bool add_shader(const char *shd);

  void set_app() { m_root = root_t::app; }
  void set_compiled() { m_compiled = true; }
  void set_dll() { m_root = root_t::dll; }
  void set_main_mod() { m_root = root_t::main_mod; }
  void set_recursed() { m_recursed = true; }
  void set_tool() { m_root = root_t::tool; }

  [[nodiscard]] constexpr const auto &executables() const noexcept {
    return m_executables;
  }
  [[nodiscard]] constexpr const auto &frameworks() const noexcept {
    return m_frameworks;
  }
  [[nodiscard]] constexpr const auto &libraries() const noexcept {
    return m_libraries;
  }
  [[nodiscard]] constexpr const auto &library_dirs() const noexcept {
    return m_library_dirs;
  }
  [[nodiscard]] constexpr const auto &mod_deps() const noexcept {
    return m_mod_deps;
  }
  [[nodiscard]] constexpr const auto &mod_impls() const noexcept {
    return m_mod_impls;
  }
  [[nodiscard]] constexpr const auto &resources() const noexcept {
    return m_resources;
  }
  [[nodiscard]] constexpr const auto &shaders() const noexcept {
    return m_shaders;
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
[[nodiscard]] bool execute(node *n);

void xlog(const node *n, const char *msg);
void errlog(const node *n, const char *msg);

node *get_node(const char *source);
node *process(const char *path);
void clear_cache();

void visit(const node *n, bool impls, void *ptr,
           void (*fn)(void *, const node *));
void visit(const node *n, bool impls, auto &&fn) {
  visit(n, impls, &fn, [](void *p, const node *nn) {
    auto pfn = static_cast<decltype(&fn)>(p);
    (*pfn)(nn);
  });
}
uint64_t visit_dirty(const node *n, void *ptr, void (*fn)(void *, const node *));
uint64_t visit_dirty(const node *n, auto &&fn) {
  return visit_dirty(n, &fn, [](void *p, const node *nn) {
    auto pfn = static_cast<decltype(&fn)>(p);
    (*pfn)(nn);
  });
}
} // namespace dag
