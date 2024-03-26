#pragma once
#include "context.hpp"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <vector>

namespace clang {
class CompilerInstance;
} // namespace clang
namespace dag {
class node;
}

class evoker {
  std::vector<std::string> m_args{};
  const dag::node *m_node{};

public:
  evoker();
  evoker(const char *verb, llvm::StringRef in, llvm::StringRef out) : evoker() {
    push_arg(verb);
    push_arg(in);
    set_out(out);
  }
  evoker &push_arg(llvm::StringRef mode) {
    m_args.push_back({mode.data(), mode.size()});
    return *this;
  }
  evoker &set_cpp_std() { return push_arg("-std=c++2b"); }
  evoker &set_out(llvm::StringRef out) {
    push_arg("-o");
    return push_arg(out);
  }

  evoker &suppress_pragmas() {
    return push_arg("-fplugin=../leco/null_pragma.dll");
  }
  evoker &add_predefs() {
    for (auto def : cur_ctx().predefs) {
      push_arg(std::string{"-D"} + def.str());
    }
    return *this;
  }
  evoker &pull_deps_from(const dag::node *n) {
    m_node = n;
    return *this;
  }

  [[nodiscard]] bool execute();
  [[nodiscard]] std::shared_ptr<clang::CompilerInstance> createCI() const;
};
