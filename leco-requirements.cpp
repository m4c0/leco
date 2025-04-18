#pragma leco tool

#include <stdio.h>
#include <string.h>

import gopt;
import popen;
import pprent;
import sim;
import strset;
import sys;
import sysstd;

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

static void run_git(const char *path) {
  char *cmd[6]{
    sysstd::strdup("git"),
    sysstd::strdup("-C"),
    sysstd::strdup(path),
    sysstd::strdup("rev-parse"),
    sysstd::strdup("HEAD"),
  };
  p::proc p{cmd};
  if (!p.gets())
    sys::die("failed to get git status");

  const char *line = p.last_line_read();
  int len = strlen(line);
  printf("%.*s ", len - 1, line);
}

int main(int argc, char **argv) try {
  auto cwd = "."_real / "out" / sys::target();

  collected.insert("../leco");
  for (auto entry : pprent::list(*cwd)) {
    auto ext = sim::path_extension(entry);
    if (ext != ".dag") continue;

    auto e = cwd / entry;
    recurse(*e);
  }

  for (const auto &s : collected) {
    run_git(s.c_str());
    puts(sim::path_filename(s.c_str()));
  }
} catch (...) {
  return 1;
}
