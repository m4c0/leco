#pragma once
#include "llvm/ADT/StringRef.h"

namespace clang {
class CompilerInstance;
} // namespace clang
namespace dag {
class node;
}

class evoker {
  std::vector<std::string> m_args{};
  const dag::node *m_node{};

public:
  evoker();
  evoker &push_arg(llvm::StringRef mode) {
    m_args.push_back({mode.data(), mode.size()});
    return *this;
  }
  evoker &set_cpp_std() { return push_arg("-std=c++2b"); }
  evoker &set_out(llvm::StringRef out) {
    push_arg("-o");
    return push_arg(out);
  }

  evoker &pull_deps_from(const dag::node *n) {
    m_node = n;
    return *this;
  }

  [[nodiscard]] bool execute();
  [[nodiscard]] std::shared_ptr<clang::CompilerInstance> createCI() const;
};
