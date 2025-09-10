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
  sim::sb exe {};
  sys::for_each_root_dag([&](auto dag, auto id, auto file) {
    if (sim::path_stem(dag) != name) return;
    if (id == 'tapp') exe = sys::read_dag_tag('edir', dag) / sim::path_filename(file);
    else exe = sim::sb { file };
  });
  if (exe == "") die("invalid target: ", name);

  *argv = *exe;
  return mct_syscall_spawn(*argv, argv);
}

static void list_wasm_targets() {
  sys::for_each_root_dag([](auto dag, auto id, auto file) {
    if (id == 'tapp') putln(*sim::path_stem(dag));
  });
}

static void run_wasm_target(const char * name, int argc, char ** argv) {
  sim::sb edir {};
  sys::for_each_root_dag([&](auto dag, auto id, auto file) {
    if (sim::path_stem(dag) != name) return;
    if (id != 'tapp') die("only app bundles are supported");
    edir = sys::read_dag_tag('edir', dag);
  });
  if (edir == "") die("invalid target: ", name);

  if (0 != mct_syscall_chdir(*edir)) die("missing bundle directory");

  // TODO: detect python3 and use a less-hardcoded path
  sys::run("python3 ../../../../leco/webserver.py");
}

static void main_wasm(int argc, char ** argv) {
  if (argc == 1) return list_wasm_targets();
  else run_wasm_target(argv[1], argc - 1, argv + 1);
}

int main(int argc, char ** argv) try {
  if (argv[argc]) return 69;
  if (sys::is_tgt_wasm()) return (main_wasm(argc, argv), 0);
  if (!sys::is_tgt_host()) die("Only host targets are supporter at the moment");

  if (argc == 1) return (list_targets(), 0);
  else return run_target(argv[1], argc - 1, argv + 1);
} catch (...) {
  return 1;
}
