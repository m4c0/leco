#pragma once
#include "llvm/ADT/SmallString.h"

namespace clang {
class CompilerInstance;
class FrontendAction;
} // namespace clang

class evoker {
  std::vector<const char *> m_args{};
  llvm::SmallString<128> m_obj{};

public:
  evoker();
  evoker &push_arg(llvm::StringRef mode) {
    m_args.push_back(mode.data());
    return *this;
  }
  evoker &set_cpp_std() { return push_arg("-std=c++2b"); }
  evoker &set_out(llvm::StringRef out);

  [[nodiscard]] bool execute();
  [[nodiscard]] std::shared_ptr<clang::CompilerInstance> createCI();

  [[nodiscard]] bool run(clang::FrontendAction &a);
  template <typename Tp> [[nodiscard]] bool run() {
    Tp action{};
    return run(action);
  }
};
