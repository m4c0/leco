#pragma once
#include "llvm/ADT/SmallString.h"

namespace clang {
class CompilerInstance;
class FrontendAction;
} // namespace clang

struct context;
class instance {
  std::shared_ptr<clang::CompilerInstance> m_ci;
  std::string m_output;

public:
  explicit instance(std::shared_ptr<clang::CompilerInstance> ci,
                    llvm::StringRef out);
  ~instance();

  [[nodiscard]] llvm::StringRef output();

  [[nodiscard]] bool run(std::unique_ptr<clang::FrontendAction> a);

  template <typename Tp> bool run() { return run(std::make_unique<Tp>()); }
};
