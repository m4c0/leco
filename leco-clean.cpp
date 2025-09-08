#pragma leco tool

import gopt;
import sys;

static bool log_all{};
static void usage() {
  sys::die(R"(
usage: ../leco/leco.exe clean [-a] [-v]

where:
      -a        remove all known deps recursively
      -v        log all removed files

)");
}

static void rm_rf(const char * p) {
  for (auto entry : pprent::list(p)) {
    if (entry[0] == '.') continue;

    auto path = sim::sb { p } / entry;
    rm_rf(*path);
  }

  if (log_all) sys::log("removing", p);
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

  if (!log_all) sys::log("removing", *path);
  rm_rf(*path);
}

int main(int argc, char **argv) try {
  bool all{};
  auto opts = gopt_parse(argc, argv, "av", [&](auto ch, auto val) {
    switch (ch) {
      case 'a': all     = true; break;
      case 'v': log_all = true; break;
      default: usage();
    }
  });
  if (opts.argc != 0) usage();

  auto cwd = "."_real;
  if (all) {
    remove_with_deps(*cwd);
  } else {
    auto tgt = cwd / "out" / sys::target();
    if (!log_all) sys::log("removing", *tgt);
    rm_rf(*tgt);
  }

  return 0;
} catch (...) {
  return 1;
}
