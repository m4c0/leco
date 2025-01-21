#pragma leco tool

import gopt;
import mtime;
import sim;
import strset;
import sys;
import sysstd;

static const char *target{};
static const char *resdir{};

static void usage() { sys::die("invalid usage"); }

static void copy_res(const char *file) {
  auto path = sim::sb { resdir } / sim::path_filename(file);
  if (mtime::of(*path) >= mtime::of(file)) return;

  sys::log("hard-linking", file);
  sys::link(file, *path);
}

static str::set added{};
static void read_dag(const char *dag) {
  if (!added.insert(dag)) return;

  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'rsrc': copy_res(file); break;
      case 'idag':
      case 'mdag': read_dag(file); break;
      default: break;
    }
  });
}

int main(int argc, char **argv) try {
  const char *input{};
  auto opts = gopt_parse(argc, argv, "o:i:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': input = val; break;
      case 'o': resdir = val; break;
      default: usage();
    }
  });

  if (!input || !resdir) usage();
  if (opts.argc != 0) usage();

  sys::mkdirs(resdir);

  auto path = sim::path_parent(input);
  target = path.path_filename();

  read_dag(input);

  return 0;
} catch (...) {
  return 1;
}
