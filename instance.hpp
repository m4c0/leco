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
  [[nodiscard]] bool run_wo_ctx(std::unique_ptr<clang::FrontendAction> a);

  template <typename Tp> bool run(bool use_ctx = true) {
    return use_ctx ? run(std::make_unique<Tp>())
                   : run_wo_ctx(std::make_unique<Tp>());
  }
};
void clear_module_path_cache();
