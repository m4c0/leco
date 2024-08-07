#pragma once
#include "die.hpp"
#include "fopen.hpp"

#include <stdint.h>
#include <string.h>

void dag_read(const char *dag, auto &&fn) {
  f::open f{dag, "r"};

  char buf[10240];
  while (!feof(*f) && fgets(buf, sizeof(buf), *f) != nullptr) {
    if (strlen(buf) < 5)
      die("invalid line in dag file");

    uint32_t *id = reinterpret_cast<uint32_t *>(buf);
    char *file = reinterpret_cast<char *>(id + 1);
    file[strlen(file) - 1] = 0;

    fn(*id, file);
  }
}
