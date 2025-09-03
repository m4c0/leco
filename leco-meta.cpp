#pragma leco tool
#include "../mct/mct-syscall.h"
import sys;

// TODO: fix hard-link bugs in OSX (my main OS, sry)
// In an ideal world, `leco.exe` would be a hard-link to
// `out/host-target/leco-driver.exe` so this tool would not be needed. For some
// reason, this does not work and `leco.exe` is only a copy in OSX. I haven't
// checked on other platforms, though.
int main(int argc, char ** argv) try {
  if (argv[argc]) return 69;
  auto cmd = sys::tool_cmd("driver");
  *argv = *cmd;
  return mct_syscall_spawn(*argv, argv);
} catch (...) {
  return 1;
}
