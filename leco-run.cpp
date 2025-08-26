#pragma leco tool
#include "../mct/mct-syscall.h"
import sys;

void list_targets() {
  sys::for_each_root_dag([](auto dag, auto id, auto file) {
    if (id == 'tmmd') return;
    putln(*sim::path_stem(dag));
  });
}

int run_target(const char * name, int argc, char ** argv) {
  bool app = false;
  sim::sb exe {};
  sys::for_each_root_dag([&](auto dag, auto id, auto file) {
    if (sim::path_stem(dag) != name) return;
    app = id == 'tapp';
    exe = sim::sb { file };
  });
  if (exe == "") die("invalid target: ", name);

  if (app) die("Support for app bundles TBD");

  *argv = *exe;
  return mct_syscall_spawn(*argv, argv);
}

int main(int argc, char ** argv) try {
  if (argv[argc]) return 69;
  if (!sys::is_tgt_host()) die("Only host targets are supporter at the moment");

  if (argc == 1) return (list_targets(), 0);
  else return run_target(argv[1], argc - 1, argv + 1);
} catch (...) {
  return 1;
}
