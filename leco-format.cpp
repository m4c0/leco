#pragma leco tool

#include <string.h>

import gopt;
import mtime;
import popen;
import sim;
import sys;
import sysstd;

static const char * fmt_cmd() {
#if __APPLE__ && __arm64__
  return "/opt/homebrew/opt/llvm/bin/clang-format";
#elif __APPLE__
  return "/usr/local/opt/llvm@16/bin/clang-format";
#elif _WIN32
  return "clang-format.exe";
#else
  return "clang-format";
#endif
}

static bool dry_run {};

static void setup_cmd(sim::sb * cmd) {
  *cmd = sim::printf("%s -i --style=file:../leco/clang-format.yaml", fmt_cmd());
  if (dry_run) *cmd += " -n --Werror";
}

static void work_from_git() {
  char * args[] {
    sysstd::strdup("git"),
    sysstd::strdup("status"),
    sysstd::strdup("--porcelain=v2"),
    0,
  };

  sim::sb cmd { 10240 };
  setup_cmd(&cmd);

  unsigned count {};
  p::proc p { args };
  while (p.gets()) {
    auto line = p.last_line_read();
    if (strncmp(line, "1 D. ", 5) == 0) {
      count++;
      continue;
    }
    if (strstr(line, ".cpp") == nullptr && strstr(line, ".mm") == nullptr) continue;

    auto file = strrchr(line, ' ') + 1;
    auto sep = strchr(file, '\t');
    int len = sep ? (sep - file) : strlen(file) - 1;

    cmd.printf(" %.*s", len, file);
    count++;
  }

  if (count == 0) {
    if (dry_run) return;

    sys::die("missing input files");
  }

  sys::run(*cmd);
}

int main(int argc, char ** argv) try {
  auto opts = gopt_parse(argc, argv, "n", [](auto ch, auto val) {
    if (ch == 'n') dry_run = true;
  });

  if (opts.argc == 0) {
    work_from_git();
    return 0;
  }

  sim::sb cmd { 10240 };
  setup_cmd(&cmd);

  for (auto i = 0; i < opts.argc; i++) {
    if (mtime::of(opts.argv[i]) == 0) sys::die("file not found: %s", opts.argv[i]);

    cmd += " ";
    cmd += opts.argv[i];
  }

  sys::run(*cmd);
} catch (...) {
  return 1;
}
