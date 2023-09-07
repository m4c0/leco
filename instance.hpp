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

  instance &add_module_path(llvm::StringRef path);

  [[nodiscard]] bool run(clang::FrontendAction *a);

  template <typename Tp> std::unique_ptr<Tp> run() {
    auto action = std::make_unique<Tp>();
    return run(action.get()) ? std::move(action) : std::unique_ptr<Tp>{};
  }
};
