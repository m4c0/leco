#pragma leco tool
#define POPEN_IMPLEMENTATION
#define SIM_IMPLEMENTATION
#include "../popen/popen.h"
#include "die.hpp"
#include "sim.hpp"

#include <stdio.h>

import mtime;
import pprent;

template <unsigned N>
static auto starts_with(const char *str, const char (&prefix)[N]) {
  return 0 == strncmp(str, prefix, N - 1);
}

int main(int argc, char **argv) try {
  for (auto file : pprent::list("..")) {
    if (file[0] == '.')
      continue;

    if (sim_path_extension(file) != nullptr)
      continue;

    sim_sbt path{};
    sim_sb_printf(&path, "../%s/.git/config", file);
    if (mtime::of(path.buffer) == 0)
      continue;

    fprintf(stderr, "-=-=-=-=-=-=-=-=-=- %s -=-=-=-=-=-=-=-=-=-\n", file);

    sim_sbt pwd{};
    sim_sb_printf(&pwd, "../%s", file);

    char *args[7]{};
    args[0] = strdup("git");
    args[1] = strdup("-C");
    args[2] = pwd.buffer;
    args[3] = strdup("status");
    args[4] = strdup("--porcelain=v2");
    args[5] = strdup("--branch");

    FILE *out;
    FILE *err;

    int res = proc_open(args, &out, &err);
    if (res != 0) {
      fprintf(stderr, "failed to get git status of [%s]", pwd.buffer);
      return 1;
    }

    char buf[1024];
    while (fgets(buf, sizeof(buf), out)) {
      if (0 == strcmp(buf, "# branch.ab +0 -0\n")) {
      } else if (starts_with(buf, "# branch.ab")) {
        printf("%s", buf);
      } else if (buf[0] == '#') {
      } else {
        printf("%s", buf);
      }
    }

    fclose(out);
    fclose(err);
  }
} catch (...) {
  return 1;
}
