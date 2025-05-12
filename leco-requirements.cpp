#pragma leco tool

#include <string.h>

import gopt;
import popen;
import sys;

static str::set all_deps{};
static str::set collected{};

static void recurse(const char * dag) {
  if (!all_deps.insert(dag)) return;

  auto path = sim::path_parent(dag);
  path.path_parent();
  path.path_parent();
  collected.insert(*path);

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
      case 'idag':
      case 'mdag': recurse(file); break;
      default: break;
    }
  });
}

int main() try {
  auto cwd = "."_real / "out" / sys::target();

  collected.insert("../leco");
  for (auto entry : pprent::list(*cwd)) {
    auto ext = sim::path_extension(entry);
    if (ext != ".dag") continue;

    auto e = cwd / entry;
    recurse(*e);
  }

  for (const auto &s : collected) {
    auto path = s.c_str();
    p::proc p { "git", "-C", path, "rev-parse", "HEAD" };
    if (!p.gets()) sys::die("failed to get git status");
    putln(*sim::sb { p.last_line_read() }.chomp(), " ", sim::path_filename(path));
  }
} catch (...) {
  return 1;
}
