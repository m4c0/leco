#pragma leco tool

import sys;

// TODO: consider merging this with leco-clang
// TODO: consider taking flag logic from leco-clang

void run(const char * parent, const char * depf, const char * incf) {
  auto mt = 0ULL;
  sys::for_each_dag(parent, true, [&](auto dag, auto id, auto file) {
    if (id == 'vers') mt = sys::max(mt, mtime::of(dag));
  });

  if (mtime::of(depf) > mt && mtime::of(incf) > mt) return;

  sys::file deps { depf, "wb" };
  sys::file incs { incf, "wb" };

  sys::for_each_dag(parent, true, [&](auto dag, auto id, auto file) {
    if      (id == 'idir') fputln(incs, "-I", file);
    else if (id == 'pcmf') {
      auto pcm = sim::sb { file };
      for (auto & c : pcm) if (c == '\\') c = '/';

      auto stem = sim::path_stem(file);
      for (auto & c : stem) if (c == '-') c = ':';

      fputfn(deps, "-fmodule-file=%s=%s", *stem, *pcm);
    }
  });
}

void run(const char * parent) {
  auto depf = sim::printf("%s/out/%s/deplist",  parent, sys::target());
  auto incf = sim::printf("%s/out/%s/includes", parent, sys::target());

  try {
    run(parent, *depf, *incf);
  } catch (...) {
    sys::remove(*depf);
    sys::remove(*incf);
    throw;
  }
}

int main() try {
  sys::strset parents {};
  sys::for_each_dag(true, [&](auto dag, auto id, auto file) {
    if (id != 'vers') return;

    auto path = sim::sb { dag }.path_parent().path_parent().path_parent();
    if (parents.insert(*path)) run(*path);
  });
} catch (...) {
  return 1;
}
