#pragma once
#include "llvm/ADT/SmallString.h"

void in2out(llvm::StringRef in, llvm::SmallVectorImpl<char> &out,
            llvm::StringRef ext);

class instance;
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
  [[nodiscard]] bool execute();
};
