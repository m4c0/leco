#pragma once
#include "llvm/ADT/SmallString.h"

namespace clang {
class FrontendAction;
}

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
  [[nodiscard]] bool run(clang::FrontendAction *a);

  [[nodiscard]] llvm::StringRef output() const { return m_obj; }

  template <typename Tp> std::unique_ptr<Tp> run() {
    auto action = std::make_unique<Tp>();
    return run(action.get()) ? std::move(action) : std::unique_ptr<Tp>{};
  }
};
