#pragma leco tool
#define SIM_IMPLEMENTATION

#include "dag2.hpp"
#include "in2out.hpp"
#include "sim.hpp"

#include <stdint.h>

import gopt;
import mtime;
import strset;
import sys;

static strset added{};

static const char *target{};
static const char *resdir{};

static void usage() { sys::die("invalid usage"); }

static void copy_res(const char *file) {
  sim_sbt path{};
  sim_sb_path_copy_append(&path, resdir, sim_path_filename(file));
  if (mtime::of(path.buffer) >= mtime::of(file))
    return;

  log("hard-linking resource", file);
  sys::link(file, path.buffer);
}

static void copy_shader(const char *file) {
  sim_sbt out{};
  sim_sb_path_copy_append(&out, resdir, sim_path_filename(file));
  sim_sb_concat(&out, ".spv");
  if (mtime::of(out.buffer) > mtime::of(file))
    return;

  log("compiling shader", file);
  sim_sbt cmd{10240};
  sim_sb_printf(&cmd, "glslangValidator -V -o %s %s", out.buffer, file);
  run(cmd.buffer);
}

static void read_dag(const char *dag) {
  if (!added.insert(dag))
    return;

  dag_read(dag, [](auto id, auto file) {
    switch (id) {
    case 'rsrc':
      copy_res(file);
      break;
    case 'shdr':
      copy_shader(file);
      break;
    case 'impl':
    case 'mdep': {
      sim_sbt ddag{};
      in2out(file, &ddag, "dag", target);
      read_dag(ddag.buffer);
      break;
    }
    default:
      break;
    }
  });
}

int main(int argc, char **argv) try {
  const char *input{};
  auto opts = gopt_parse(argc, argv, "o:i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    case 'o':
      resdir = val;
      break;
    default:
      usage();
    }
  });

  if (!input || !resdir)
    usage();
  if (opts.argc != 0)
    usage();

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, input);
  target = sim_sb_path_filename(&path);

  read_dag(input);

  return 0;
} catch (...) {
  return 1;
}
