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
  llvm::StringSet<> m_frameworks{};
  llvm::StringSet<> m_mod_deps{};
  llvm::StringSet<> m_mod_impls{};
  bool m_compiled{};
  root_t m_root{};
  bool m_recursed{};

public:
  explicit node(llvm::StringRef n);

  void add_mod_dep(llvm::StringRef mod_name);
  void add_mod_impl(llvm::StringRef mod_impl);
  void add_framework(llvm::StringRef fw) { m_frameworks.insert(fw); }

  void set_app() { m_root = root_t::app; }
  void set_compiled() { m_compiled = true; }
  void set_main_mod() { m_root = root_t::main_mod; }
  void set_recursed() { m_recursed = true; }
  void set_tool() { m_root = root_t::tool; }

  [[nodiscard]] constexpr const auto &frameworks() const noexcept {
    return m_frameworks;
  }
  [[nodiscard]] constexpr const auto &mod_deps() const noexcept {
    return m_mod_deps;
  }
  [[nodiscard]] constexpr const auto &mod_impls() const noexcept {
    return m_mod_impls;
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
};

node *get_node(llvm::StringRef source);
node *process(llvm::StringRef path);
void clear_cache();
} // namespace dag
