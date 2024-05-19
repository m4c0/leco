#pragma once
#include "die.hpp"

#include <stdio.h>

namespace f {
class open {
  FILE *m_f;

public:
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
  ~open() { fclose(m_f); }

  constexpr FILE *operator*() { return m_f; }
};
} // namespace f
