#pragma leco tool

#define MTIME_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../mtime/mtime.h"
#include "dag2.hpp"
#include "die.hpp"
#include "in2out.hpp"
#include "sim.hpp"

#include <filesystem>
#include <set>
#include <string>

import gopt;

static const char *exedir{};
static const char *target{};

static void usage() { die("invalid usage"); }

static void copy_exe(const char *input) {
  sim_sbt path{};
  sim_sb_copy(&path, exedir);
  sim_sb_path_append(&path, sim_path_filename(input));

  if (mtime_of(path.buffer) >= mtime_of(input))
    return;

  log("copying", path.buffer);

  if (0 != remove(path.buffer)) {
    // Rename original file. This is a "Windows-approved" way of modifying an
    // open executable.
    sim_sbt bkp{};
    sim_sb_copy(&bkp, path.buffer);
    sim_sb_concat(&bkp, ".bkp");
    remove(bkp.buffer);
    rename(path.buffer, bkp.buffer);
  }
  std::filesystem::copy_file(input, path.buffer);
}

static void copy_bdep(const char *src) {
  sim_sbt dag{};
  in2out(src, &dag, "dag", target);

  dag_read(dag.buffer, [&](auto id, auto file) {
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

static std::set<std::string> added{};
static void read_dag(const char *dag) {
  auto [_, inserted] = added.insert(dag);
  if (!inserted)
    return;

  dag_read(dag, [](auto id, auto file) {
    switch (id) {
    case 'bdep':
      copy_bdep(file);
      break;
    case 'dlls':
      copy_exe(file);
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

  auto opts = gopt_parse(argc, argv, "i:o:", [&](auto ch, auto val) {
    switch (ch) {
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
