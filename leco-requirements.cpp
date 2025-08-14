#pragma leco tool

import popen;
import sys;

int main() try {
  sys::strset collected {};
  collected.insert("../leco"); // Marking as dependency for any leco-based software

  sys::for_each_tag_in_dags('vers', true, [&](auto dag, auto file) {
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
