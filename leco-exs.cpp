#pragma leco tool

#include "in2out.hpp"
#include "sim.hpp"

#include <stdio.h>

import gopt;
import mtime;
import strset;
import sys;

static const char *exedir{};
static const char *target{};
static const char *ext{"exe"};

static void usage() { sys::die("invalid usage"); }

static void copy_exe(const char *input) {
  sim_sbt path{};
  sim_sb_copy(&path, exedir);
  sim_sb_path_append(&path, sim_path_filename(input));
  sim_sb_path_set_extension(&path, ext);

  if (mtime::of(path.buffer) >= mtime::of(input))
    return;

  sys::log("copying", path.buffer);

  if (0 != remove(path.buffer)) {
    // Rename original file. This is a "Windows-approved" way of modifying an
    // open executable.
    sim_sbt bkp{};
    sim_sb_copy(&bkp, path.buffer);
    sim_sb_concat(&bkp, ".bkp");
    remove(bkp.buffer);
    rename(path.buffer, bkp.buffer);
  }
  sys::link(input, path.buffer);
}

static void copy_bdep(const char *src) {
  sim_sbt dag{};
  in2out(src, &dag, "dag", target);

  sys::dag_read(dag.buffer, [&](auto id, auto file) {
    switch (id) {
    case 'tdll':
    case 'tool':
    case 'tapp':
      copy_exe(file);
      break;
    default:
      break;
    }
  });
}

static str::set added{};
static void read_dag(const char *dag) {
  if (!added.insert(dag))
    return;

  sys::dag_read(dag, [](auto id, auto file) {
    switch (id) {
    case 'bdep':
      copy_bdep(file);
      break;
    case 'dlls':
      copy_exe(file);
      break;
    case 'idag':
    case 'mdag': {
      read_dag(file);
      break;
    }
    default:
      break;
    }
  });
}

int main(int argc, char **argv) try {
  const char *input{};

  auto opts = gopt_parse(argc, argv, "e:i:o:", [&](auto ch, auto val) {
    switch (ch) {
    case 'e':
      ext = val;
      break;
    case 'i':
      input = val;
      break;
    case 'o':
      exedir = val;
      break;
    default:
      usage();
      break;
    }
  });
  if (!input || !exedir)
    usage();
  if (opts.argc != 0)
    usage();

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, input);
  target = sim_sb_path_filename(&path);

  sim_sbt exe{};
  sim_sb_copy(&exe, input);
  sim_sb_path_set_extension(&exe, "exe");
  copy_exe(exe.buffer);

  read_dag(input);
} catch (...) {
  return 1;
}
