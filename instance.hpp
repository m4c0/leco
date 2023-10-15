#pragma once
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/ADT/SmallString.h"

namespace clang {
class FrontendAction;
} // namespace clang

struct context;
class instance {
  std::shared_ptr<clang::CompilerInstance> m_ci;
  std::string m_output;
  // m_ci->getHeaderSearchOpts().AddPrebuiltModulePath(p);

public:
  explicit instance(std::shared_ptr<clang::CompilerInstance> ci,
                    llvm::StringRef out)
      : m_ci{std::move(ci)}, m_output{out.str()} {}

  [[nodiscard]] bool run(std::unique_ptr<clang::FrontendAction> a) {
    return m_ci ? m_ci->ExecuteAction(*a) : false;
  }

  template <typename Tp> bool run() { return run(std::make_unique<Tp>()); }
};
