#pragma leco tool
#define GOPT_IMPLEMENTATION
#define MTIME_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "../mtime/mtime.h"
#include "die.hpp"
#include "fopen.hpp"
#include "in2out.hpp"
#include "sim.hpp"
#include "log.hpp"

#include <filesystem>
#include <stdint.h>
#include <set>
#include <string>

static std::set<std::string> added{};

static const char *target{};
static const char *exedir{};
static const char *resdir{};

static void usage() { die("invalid usage"); }

static void copy_shader(const char *file) {
  sim_sbt out{};
  sim_sb_path_copy_append(&out, resdir, sim_path_filename(file));
  sim_sb_concat(&out, ".spv");
  if (mtime_of(out.buffer) > mtime_of(file))
    return;

  log("compiling shader", file);
  sim_sbt cmd{10240};
  sim_sb_printf(&cmd, "glslangValidator -V -o %s %s", out.buffer, file);
  run(cmd.buffer);
}

static void copy(const char * what, const char *file, const char *out) {
  if (mtime_of(out) > mtime_of(file))
    return;

  log(what, file);
  if (0 != remove(out)) {
    // Rename original file. This is a "Windows-approved" way of modifying an
    // open executable.
    sim_sbt bkp{};
    sim_sb_copy(&bkp, out);
    sim_sb_concat(&bkp, ".bkp");
    remove(bkp.buffer);
    rename(out, bkp.buffer);
  }
  std::filesystem::copy_file(file, out);
}

static void copy_dll(const char *file) {
}

static void copy_bdep(const char *file) {
}

static void copy_res(const char *file) {
  sim_sbt path{};
  sim_sb_path_copy_append(&path, resdir, sim_path_filename(file));
  copy("copying resource", file, path.buffer);
}

static void read_dag(const char *dag) {
  auto [_, inserted] = added.insert(dag);
  if (!inserted)
    return;

  FILE *f{};
  if (0 != fopen_s(&f, dag, "r"))
    die("dag file not found: [%s]\n", dag);

  char buf[10240];
  while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
    if (strlen(buf) < 5)
      die("invalid line in dag file");

    uint32_t *id = reinterpret_cast<uint32_t *>(buf);
    char *file = reinterpret_cast<char *>(id + 1);
    file[strlen(file) - 1] = 0;

    switch (*id) {
    case 'bdep':
      copy_bdep(file);
      break;
    case 'dlls':
      copy_dll(file);
      break;
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
  }

  fclose(f);
}

int main(int argc, char **argv) try {
  struct gopt opts;
  GOPT(opts, argc, argv, "r:e:i:");

  const char *input{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    case 'e':
      exedir = val;
      break;
    case 'r':
      resdir = val;
      break;
    default:
      usage();
    }
  }
  if (!input || !exedir || !resdir)
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
