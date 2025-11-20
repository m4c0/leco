#pragma leco tool

import popen;
import sys;

[[noreturn]] void usage() {
  die(R"(
    Creates a list of LECO modules used by the current project.

    Usage: ../leco/leco.exe requirements [-v]

    Where:
        -v  Marks git repo that are not clean
)");
}

int main(int argc, char ** argv) try {
  bool verbose = true;

  const auto shift = [&] { return argc > 1 ? (argc--, *++argv) : nullptr; };
  while (auto val = shift()) {
    if ("-v"_s == val) verbose = true;
    else usage();
  }

  sys::strset collected {};
  collected.insert(*"../leco"_real); // Marking as dependency for any leco-based software

  sys::for_each_tag_in_dags('vers', true, [&](auto dag, auto file) {
    auto path = sim::path_parent(dag);
    path.path_parent();
    path.path_parent();
    collected.insert(*path);
  });

  for (const auto &s : collected) {
    auto path = s.c_str();
    p::proc p { "git", "-C", path, "rev-parse", "HEAD" };
    if (!p.gets()) die("failed to get git status");
    putan(p.last_line_read(), sim::path_filename(path));

    if (verbose) {
      p::proc p { "git", "-C", path, "status", "--porcelain=v2", "--branch" };
      while (p.gets()) {
        auto line = sv::unsafe(p.last_line_read());

        if ("# branch.ab +0 -0"_sv == line) continue;
        else if (line.starts_with("# branch.")) continue;
        else putln(line);
      }
    }
  }
} catch (...) {
  return 1;
}
