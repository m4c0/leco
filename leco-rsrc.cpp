#pragma leco tool

import gopt;
import sys;

static const char *target{};
static const char *resdir{};

static void usage() { sys::die("invalid usage"); }

static void copy_res(const char *file) {
  auto path = sim::sb { resdir } / sim::path_filename(file);
  if (mtime::of(*path) >= mtime::of(file)) return;

  sys::log("hard-linking", file);
  sys::link(file, *path);
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

  sys::recurse_dag(input, [](auto dag, auto id, auto file) {
    if (id == 'rsrc') copy_res(file);
  });

  return 0;
} catch (...) {
  return 1;
}
