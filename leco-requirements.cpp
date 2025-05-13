#pragma leco tool

import popen;
import sys;

int main() try {
  str::set collected {};
  collected.insert("../leco"); // Marking as dependency for any leco-based software

  sys::for_each_dag(true, [&](auto dag, auto id, auto file) {
    if (id != 'vers') return;

    auto path = sim::path_parent(dag);
    path.path_parent();
    path.path_parent();
    collected.insert(*path);
  });

  for (const auto &s : collected) {
    auto path = s.c_str();
    p::proc p { "git", "-C", path, "rev-parse", "HEAD" };
    if (!p.gets()) sys::die("failed to get git status");
    putln(*sim::sb { p.last_line_read() }.chomp(), " ", sim::path_filename(path));
  }
} catch (...) {
  return 1;
}
