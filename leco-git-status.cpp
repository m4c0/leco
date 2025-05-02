#pragma leco tool

#include <stdio.h>
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

    char *args[7] {};
    args[0] = sysstd::strdup("git");
    args[1] = sysstd::strdup("-C");
    args[2] = *pwd;
    args[3] = sysstd::strdup("status");
    args[4] = sysstd::strdup("--porcelain=v2");
    args[5] = sysstd::strdup("--branch");

    p::proc gs{args};

    bool printing{};
    const auto enable_printer = [&] {
      if (printing) return;
      fprintf(stderr, "-=-=-=-=-=-=-=-=-=- %s -=-=-=-=-=-=-=-=-=-\n", file);
      printing = true;
    };

    while (gs.gets()) {
      auto buf = gs.last_line_read();
      if (0 == strcmp(buf, "# branch.ab +0 -0\n")) {
      } else if (starts_with(buf, "# branch.ab")) {
        enable_printer();
        sys::runf("git -C %s log --branches --not --remotes --format=oneline", *pwd);
      } else if (buf[0] == '#') {
      } else {
        enable_printer();
        fputs(buf, stderr);
      }
    }
  }
} catch (...) {
  return 1;
}
