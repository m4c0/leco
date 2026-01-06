#pragma leco tool
#include <stdio.h>

import jojo;
import pprent;
import sv;
import sys;
import zipline;

static void usage() {
  die(R"(
Creates ZIP files out of each "app". Useful for distribution of Windows and
WASM targets.

Usage: ../leco/leco.exe zip
)");
}

class file_writer : public zipline::writer {
  sys::file m_f;

public:
  explicit file_writer(const char * fname) : m_f { fname, "wb" } {}

  void write(const void * ptr, unsigned sz) {
    if (1 != fwrite(ptr, sz, 1, m_f)) die("failed to write zip file");
  }
  unsigned tell() {
    return ftell(m_f);
  }
};

static void run(const char * dag, const char * _) {
  auto path = sim::sb { dag }.path_extension("app");
  auto zip = sim::sb { dag }.path_extension("zip");

  sys::log("bundling", *zip);
  // TODO: use DAG to find which files should be bundled

  file_writer fw { *zip };
  zipline::zipwriter w { &fw };
  for (auto f : pprent::list(*path)) {
    if (f[0] == '.') continue;
    
    auto fn = sv::unsafe(f);
    if (fn.ends_with(".log")) continue;

    auto fname = path / f;
    w.write_file(fn, jojo::slurp(fname));
  }
  w.write_cd();
}

int main(int argc, char **argv) try {
  if (sys::is_tgt_apple()) usage();
  sys::for_each_tag_in_dags('tapp', false, &run);
} catch (...) {
  return 1;
}
