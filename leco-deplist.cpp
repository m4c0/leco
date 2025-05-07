#pragma leco tool
#include <stdio.h>

import sys;

// TODO: consider merging this with leco-clang
// TODO: consider taking flag logic from leco-clang

int main() try {
  sys::for_each_dag(true, [](auto dag, auto id, auto file) {
    if (id != 'vers') return;

    sim::sb output { dag };

    // TODO: fix the following bug
    // 1. "leco"
    // 2. Add a new module in a dep (ex: added "print" to "sys")
    // 3. "leco" again
    // We are failing with "failed to find module file for module 'print'"

    output.path_extension("deps");
    if (mtime::of(dag) > mtime::of(*output)) {
      auto out = sys::fopen(*output, "wb");
      sys::recurse_dag(dag, [&](auto dag, auto id, auto file) {
        if (id != 'pcmf') return;

        auto pcm  = sim::sb { file };
        for (auto & c : pcm) if (c == '\\') c = '/';

        auto stem = sim::path_stem(file);
        for (auto & c : stem) if (c == '-') c = ':';

        fprintf(out, "-fmodule-file=%s=%s\n", *stem, *pcm);
      });
      fclose(out);
    }

    output.path_extension("incs");
    if (mtime::of(dag) > mtime::of(*output)) {
      auto out = sys::fopen(*output, "wb");
      sys::dag_read(dag, [&](auto id, auto file) {
        if (id == 'idir') fprintf(out, "-I%s\n", file);
      });
      fclose(out);
    }
  });

  return 0;
} catch (...) {
  return 1;
}
