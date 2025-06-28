#pragma leco tool
#include "../mct/mct-syscall.h"
import sys;

static void chdir(const char * dir) {
  if (0 != mct_syscall_chdir(dir)) sys::die("Directory not found: [%s]\n", dir);
}

extern "C" int puts(const char *);
int main(int argc, char ** argv) try {
  if (argv[argc]) return 69;

  const auto shift = [&] { return argc ? (argc--, *++argv) : nullptr; };
  while (auto val = shift())
    if ("-C"_s == val) chdir(shift());
    else break;

  auto cmd = sys::tool_cmd(*argv ? *argv : "");
  if (!mtime::of(*cmd)) {
    argv--;
    cmd = sys::tool_cmd("driver");
  }
  *argv = *cmd;
  return mct_syscall_spawn(*argv, argv);
} catch (...) {
  return 1;
}
