#pragma once
#include "clang/Frontend/FrontendAction.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"

namespace dag {
class node {
  llvm::SmallString<256> m_source;
  llvm::StringSet<> m_mod_deps{};
  bool m_root{};

public:
  explicit node(llvm::StringRef n);

  void add_mod_dep(llvm::StringRef name) { m_mod_deps.insert(name); }
  void set_root() { m_root = true; }

  [[nodiscard]] constexpr const auto &mod_deps() const noexcept {
    return m_mod_deps;
  }
  [[nodiscard]] constexpr bool root() const noexcept { return m_root; }
  [[nodiscard]] constexpr llvm::StringRef source() const noexcept {
    return m_source;
  }
};

std::unique_ptr<node> process(llvm::StringRef path);
} // namespace dag
