#pragma leco tool

import sys;

static void copy_res(const sim::sb & resdir, const char * file) {
  auto path = resdir / sim::path_filename(file);
  sys::link(file, *path);
}

static void copy_res_dir(const sim::sb & resdir, const char * rdir) {
  sim::sb rin_dir { rdir };
  for (auto p : pprent::list(rdir)) {
    if (p[0] == '.') continue;

    auto file = rin_dir / p;
    copy_res(resdir, *file);
  }
}

int main(int argc, char **argv) try {
  sys::for_each_root_dag([](auto dag, auto id, auto file) {
    if (id != 'tapp') return;

    sim::sb rdir = sys::read_dag_tag('rdir', dag);
    if (rdir == "") die("app dag without resource dir");

    sys::mkdirs(*rdir);
    sys::recurse_dag(dag, [&](auto dag, auto id, auto file) {
      if (id == 'rdir') copy_res_dir(rdir, file);
      if (id == 'rsrc') copy_res(rdir, file);
    });
  });

  return 0;
} catch (...) {
  return 1;
}
