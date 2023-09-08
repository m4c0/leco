#pragma once
#include "llvm/ADT/SmallString.h"

namespace clang {
class CompilerInstance;
class FrontendAction;
} // namespace clang

class instance {
  std::shared_ptr<clang::CompilerInstance> m_ci;
  std::string m_output;

public:
  explicit instance(std::shared_ptr<clang::CompilerInstance> ci,
                    llvm::StringRef out);
  ~instance();

  [[nodiscard]] llvm::StringRef output();

  [[nodiscard]] bool run(std::unique_ptr<clang::FrontendAction> a,
                         bool wrapped);

  template <typename Tp> bool run() {
    return run(std::make_unique<Tp>(), true);
  }
  template <typename Tp> bool run_raw() {
    return run(std::make_unique<Tp>(), false);
  }
};
