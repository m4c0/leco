#pragma leco tool
#define SIM_IMPLEMENTATION
#include "sim.hpp"

#if _WIN32
#include <process.h>
#define execv _execv
#else
#include <unistd.h>
#endif

import sys;

int main(int argc, char ** argv) {
  sim_sbt cmd { 10240 };
  sim_sb_path_copy_real(&cmd, ".");
  sim_sb_path_parent(&cmd);
  sim_sb_path_append(&cmd, "leco");
  sim_sb_path_append(&cmd, "leco.exe");

  for (auto i = 1; i < argc; i++) {
    sim_sb_printf(&cmd, " %s", argv[i]);
  }

  sys::run(cmd.buffer);

  // We need to execlp instead of "system" because Python does weird stuff to
  // signals. Using Ctrl-C to stop the process sometimes leaves python running
  // in the background.
  return execlp("python3", "python3", "../leco/webserver.py", nullptr);
}
