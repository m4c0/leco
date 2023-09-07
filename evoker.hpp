#pragma once
#include "llvm/ADT/SmallString.h"

namespace clang {
class CompilerInstance;
class FrontendAction;
} // namespace clang

class instance {
  std::unique_ptr<clang::CompilerInstance> m_ci;
  std::string m_output;

public:
  explicit instance(std::unique_ptr<clang::CompilerInstance> ci, std::string o);
  ~instance();

  [[nodiscard]] llvm::StringRef output();

  [[nodiscard]] bool run(clang::FrontendAction *a);

  template <typename Tp> std::unique_ptr<Tp> run() {
    auto action = std::make_unique<Tp>();
    return run(action.get()) ? std::move(action) : std::unique_ptr<Tp>{};
  }
};

class evoker {
  std::vector<const char *> m_args{};
  llvm::SmallString<128> m_obj{};

public:
  evoker();
  evoker &push_arg(llvm::StringRef mode) {
    m_args.push_back(mode.data());
    return *this;
  }
  evoker &set_inout(llvm::StringRef in, llvm::StringRef ext);

  [[nodiscard]] instance build();
};
