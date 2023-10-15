#pragma once
#include "clang/Frontend/FrontendAction.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"

namespace dag {
enum class root_t {
  none,
  main_mod,
  tool,
  app,
};

class node {
  llvm::SmallString<256> m_source;
  llvm::SmallString<256> m_target;
  llvm::StringSet<> m_executables{};
  llvm::StringSet<> m_frameworks{};
  llvm::StringSet<> m_libraries{};
  llvm::StringSet<> m_mod_deps{};
  llvm::StringSet<> m_mod_impls{};
  llvm::StringSet<> m_resources{};
  llvm::StringSet<> m_shaders{};
  bool m_compiled{};
  root_t m_root{};
  bool m_recursed{};

public:
  explicit node(llvm::StringRef n);

  [[nodiscard]] bool add_executable(llvm::StringRef e);
  void add_framework(llvm::StringRef fw) { m_frameworks.insert(fw); }
  void add_library(llvm::StringRef lib) { m_libraries.insert(lib); }
  [[nodiscard]] bool add_mod_dep(llvm::StringRef mod_name);
  [[nodiscard]] bool add_mod_impl(llvm::StringRef mod_impl);
  [[nodiscard]] bool add_resource(llvm::StringRef res);
  [[nodiscard]] bool add_shader(llvm::StringRef shd);

  void set_app() { m_root = root_t::app; }
  void set_compiled() { m_compiled = true; }
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

  [[nodiscard]] constexpr bool app() const noexcept {
    return m_root == root_t::app;
  }
  [[nodiscard]] constexpr bool compiled() const noexcept { return m_compiled; }
  [[nodiscard]] constexpr bool recursed() const noexcept { return m_recursed; }
  [[nodiscard]] constexpr bool root() const noexcept {
    return m_root != root_t::none;
  }
  [[nodiscard]] constexpr bool tool() const noexcept {
    return m_root == root_t::tool;
  }

  [[nodiscard]] constexpr llvm::StringRef source() const noexcept {
    return m_source;
  }
  [[nodiscard]] constexpr llvm::StringRef target() const noexcept {
    return m_target;
  }
};

node *get_node(llvm::StringRef source);
node *process(llvm::StringRef path);
void clear_cache();

void visit(const node *n, auto &&fn) {
  llvm::StringSet<> visited{};

  const auto rec = [&](auto rec, auto *n) {
    if (visited.contains(n->source()))
      return;

    for (auto &d : n->mod_deps()) {
      rec(rec, get_node(d.first()));
    }

    fn(n);
    visited.insert(n->source());
  };
  rec(rec, n);
}
llvm::sys::TimePoint<> visit_dirty(const node *n,
                                   llvm::function_ref<void(const node *)> fn);
} // namespace dag
