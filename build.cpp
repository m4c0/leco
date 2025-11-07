#define MCT_STAT_IMPLEMENTATION
#define MCT_SYSCALL_IMPLEMENTATION
#define SIM_IMPLEMENTATION
#define SYSSTD_IMPLEMENTATION
#include "sim.h"
#include "targets.hpp"
#include "../mct/mct-stat.h"
#include "../mct/mct-syscall.h"

#include <stdio.h>
#include <stdlib.h>

#define SEP SIM_PATHSEP_S

#define CLANG CLANG_CMD

#define CPPSTD " -std=c++2c"
#define PCMFL " --precompile" CPPSTD
#define PMP " -fprebuilt-module-path=out/" HOST_TARGET

static auto sb_alloc() {
  sim_sb sb {};
  sim_sb_new(&sb, 102400);
  return sb;
}

static auto object_args = sb_alloc();
static auto module_args = sb_alloc();

static void run(const char * cmd) {
  if (0 == system(cmd)) return;
  fprintf(stderr, "command failed: %s\n", cmd);
  throw 0;
}

static void mkout(const char * name) {
  static auto sb = sb_alloc();

  sim_sb_path_copy_append(&sb, "..", name);
  sim_sb_path_append(&sb, "out");
  mct_syscall_mkdir(sb.buffer);

  sim_sb_path_append(&sb, HOST_TARGET);
  mct_syscall_mkdir(sb.buffer);
}

static void do_module(const char * name) {
  mkout(name);

  static auto mod_sb = sb_alloc();
  sim_sb_copy(&mod_sb, "");
  sim_sb_printf(&mod_sb, "../%s/out/" HOST_TARGET "/%s.pcm", name, name);

  static auto sb = sb_alloc();
  sim_sb_copy(&sb, "");
  sim_sb_printf(&sb,
      CLANG " " PCMFL " ../%s/%s.cppm -o %s %s",
      name, name, mod_sb.buffer, module_args.buffer);
  run(sb.buffer);

  sim_sb_printf(&module_args, " -fmodule-file=%s=%s", name, mod_sb.buffer);
  sim_sb_printf(&object_args, " %s", mod_sb.buffer);
}

static void local_module(const char * name) {
  static auto sb = sb_alloc();
  sim_sb_copy(&sb, "");
  sim_sb_printf(&sb,
      CLANG PCMFL PMP " %s.cppm -o out/" HOST_TARGET "/%s.pcm %s",
      name, name, module_args.buffer);
  run(sb.buffer);

  sim_sb_printf(&object_args, " out/" HOST_TARGET "/%s.pcm", name);
}

static void tool(const char * name) {
  printf("Building %s\n", name);

  static auto sb = sb_alloc();
  sim_sb_copy(&sb, "");
  sim_sb_printf(&sb,
      CLANG " leco-%s.cpp -o out/" HOST_TARGET "/leco-%s.exe" PMP CPPSTD " %s %s",
      name, name, module_args.buffer, object_args.buffer);
  run(sb.buffer);
}

int try_main() {
  mkout("leco");

  puts("Building dependencies");
  do_module("hay");
  do_module("mtime");
  do_module("no");
  do_module("popen");
  do_module("pprent");
  do_module("print");
  do_module("sv");
  do_module("sysstd");

  puts("Building core modules");
  local_module("c");
  local_module("sim");
  local_module("sys");

  tool("clang");
  tool("dagger");
  tool("deplist");
  tool("driver");
  tool("link");
  tool("obj");
  tool("pcm");
  tool("pcm2obj");

  puts("Self-hosted build of final stage");
  run("." SEP "out" SEP HOST_TARGET SEP "leco-driver.exe");

  puts("Hard-linking meta caller");
  remove("leco.exe");
  mct_syscall_link("out" SEP HOST_TARGET SEP "leco-meta.exe", "leco.exe");

  puts("Doney-devito");
  return 0;
}

int main() try {
  if (mct_stat_mtime("build.cpp") > mct_stat_mtime("build.exe")) {
    puts("Rebuilding self");
    remove("build.old");
    run("clang++ -std=c++20 build.cpp -o build.new");
    rename("build.exe", "build.old");
    rename("build.new", "build.exe");
    run("." SEP "build.exe");
    return 0;
  }
  return try_main();
} catch (...) {
  return 1;
}
