#pragma once
#include "llvm/ADT/SmallString.h"

namespace clang {
class CompilerInstance;
class FrontendAction;
} // namespace clang
namespace dag {
class node;
}

class evoker {
  std::vector<const char *> m_args{};
  const dag::node *m_node{};

public:
  evoker();
  evoker &push_arg(llvm::StringRef mode) {
    m_args.push_back(mode.data());
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
  [[nodiscard]] std::shared_ptr<clang::CompilerInstance> createCI();

  [[nodiscard]] bool run(std::unique_ptr<clang::FrontendAction> a);
  template <typename Tp> [[nodiscard]] bool run() {
    return run(std::make_unique<Tp>());
  }
};
