#pragma once
#include <memory>
#include <string>
#include <vector>

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

public:
  evoker();

  evoker &push_arg(const char *mode) {
    m_args.push_back(mode);
    return *this;
  }

  [[nodiscard]] vex prepare_args();
  [[nodiscard]] bool execute();
};
