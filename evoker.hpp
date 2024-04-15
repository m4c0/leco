#pragma once
#include <memory>
#include <string>
#include <vector>

namespace clang {
class CompilerInstance;
} // namespace clang
namespace dag {
class node;
}

class vex {
  std::string m_argfile;

public:
  explicit vex(std::string a) : m_argfile{a} {}
  ~vex();

  vex(const vex &) = delete;
  vex(vex &&) = delete;
  vex &operator=(const vex &) = delete;
  vex &operator=(vex &&) = delete;

  [[nodiscard]] operator bool() const;

  [[nodiscard]] const char * argument_file() const { return m_argfile.c_str(); }
};

class evoker {
  std::vector<std::string> m_args{};
  const dag::node *m_node{};

public:
  evoker();
  evoker(const char *verb, const char *in, const char *out);

  evoker &push_arg(const char *mode) {
    m_args.push_back(mode);
    return *this;
  }
  evoker &set_cpp();
  evoker &set_out(const char *out) {
    push_arg("-o");
    return push_arg(out);
  }

  evoker &suppress_pragmas();
  evoker &add_predefs();
  evoker &pull_deps_from(const dag::node *n) {
    m_node = n;
    return *this;
  }

  [[nodiscard]] vex prepare_args();
  [[nodiscard]] bool execute();
  [[nodiscard]] std::shared_ptr<clang::CompilerInstance> createCI() const;
};
