#pragma leco tool

#include <string.h>

import popen;
import sys;

template <unsigned N>
static auto starts_with(const char *str, const char (&prefix)[N]) {
  return 0 == strncmp(str, prefix, N - 1);
}

int main() try {
  for (auto file : pprent::list("..")) {
    if (file[0] == '.') continue;

    if (sim::path_extension(file).len) continue;

    auto path = sim::printf("../%s/.git/config", file);
    if (mtime::of(*path) == 0) continue;

    auto pwd = sim::printf("../%s", file);

    p::proc gs { "git", "-C", *pwd, "status", "--porcelain=v2", "--branch" };

    bool printing{};
    const auto enable_printer = [&] {
      if (printing) return;
      errln("-=-=-=-=-=-=-=-=-=- ", file, " -=-=-=-=-=-=-=-=-=-");
      printing = true;
    };

    while (gs.gets()) {
      auto buf = gs.last_line_read();
      if (0 == strcmp(buf, "# branch.ab +0 -0")) {
      } else if (starts_with(buf, "# branch.ab")) {
        enable_printer();
        sys::runf("git -C %s log --branches --not --remotes --format=oneline", *pwd);
      } else if (buf[0] == '#') {
      } else {
        enable_printer();
        err(buf);
      }
    }
  }
} catch (...) {
  return 1;
}
