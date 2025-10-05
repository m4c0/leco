#pragma leco tool

import gopt;
import sys;

static void usage() {
  die(R"(
Cleans LECO build directories.

usage: ../leco/leco.exe clean [-a]

where:
      -a: remove all known deps recursively

)");
}

static void rm_rf(const char * p) {
  for (auto entry : pprent::list(p)) {
    if (entry[0] == '.') continue;

    auto path = sim::sb { p } / entry;
    rm_rf(*path);
  }
  sys::remove(p);
}

static sys::strset temp{};
static void remove_with_deps(const char * p) {
  if (!temp.insert(p)) return;

  auto path = sim::sb { p } / "out" / sys::target();

  for (auto entry : pprent::list(*path)) {
    auto dag = path / entry;

    if (sim::path_extension(*dag) != ".dag") continue;

    sys::dag_read(*dag, [&](auto id, auto file) {
      switch (id) {
        case 'idag':
        case 'mdag': {
          auto p = sim::path_parent(file);
          p.path_parent();
          p.path_parent();
          remove_with_deps(*p);
          break;
        }
        default: break;
      }
    });
  }

  rm_rf(*path);
}

int main(int argc, char **argv) try {
  bool all{};
  auto opts = gopt_parse(argc, argv, "av", [&](auto ch, auto val) {
    switch (ch) {
      case 'a': all = true; break;
      default: usage();
    }
  });
  if (opts.argc != 0) usage();

  auto cwd = "."_real;
  if (all) {
    remove_with_deps(*cwd);
  } else {
    auto tgt = cwd / "out" / sys::target();
    sys::log("removing", *tgt);
    rm_rf(*tgt);
  }

  return 0;
} catch (...) {
  return 1;
}
