#pragma once
#include "die.hpp"

#include <stdio.h>

namespace f {
class open {
  FILE *m_f{};

public:
  constexpr open() = default;
  open(const char *name, const char *mode) {
#ifdef _WIN32
    if (0 != fopen_s(&m_f, name, mode))
      die("could not open file [%s]", name);
#else
    m_f = fopen(name, mode);
    if (m_f == nullptr)
      die("could not open file [%s]", name);
#endif
  }
  ~open() {
    if (m_f)
      fclose(m_f);
  }

  open(const open &o) = delete;
  open &operator=(const open &o) = delete;
  open(open &&o) : m_f{o.m_f} { o.m_f = nullptr; }
  open &operator=(open &&o) {
    if (m_f)
      fclose(m_f);
    m_f = o.m_f;
    o.m_f = nullptr;
    return *this;
  }

  constexpr FILE *operator*() { return m_f; }
};
} // namespace f
