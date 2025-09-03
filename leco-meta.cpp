#pragma leco tool
#include "../mct/mct-syscall.h"
import sys;

int main(int argc, char ** argv) try {
  if (argv[argc]) return 69;
  auto cmd = sys::tool_cmd("driver");
  *argv = *cmd;
  return mct_syscall_spawn(*argv, argv);
} catch (...) {
  return 1;
}
