#pragma once
#include "clang/Frontend/FrontendAction.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"

namespace dag {
class node {
  llvm::SmallString<256> m_source;
  llvm::StringSet<> m_mod_deps{};
  llvm::StringSet<> m_mod_impls{};
  bool m_root{};
  bool m_recursed{};

public:
  explicit node(llvm::StringRef n);

  void add_mod_dep(llvm::StringRef mod_name);
  void add_mod_impl(llvm::StringRef mod_impl);
  void set_recursed() { m_recursed = true; }
  void set_root() { m_root = true; }

  [[nodiscard]] constexpr const auto &mod_deps() const noexcept {
    return m_mod_deps;
  }
  [[nodiscard]] constexpr const auto &mod_impls() const noexcept {
    return m_mod_impls;
  }
  [[nodiscard]] constexpr bool recursed() const noexcept { return m_recursed; }
  [[nodiscard]] constexpr bool root() const noexcept { return m_root; }
  [[nodiscard]] constexpr llvm::StringRef source() const noexcept {
    return m_source;
  }
};

node *get_node(llvm::StringRef source);
node *process(llvm::StringRef path);
void clear_cache();
} // namespace dag
