#pragma leco tool
#define GOPT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "die.hpp"
#include "fopen.hpp"
#include "in2out.hpp"
#include "sim.hpp"

#include <set>
#include <string>

static const char *target{};
static FILE *out{};
static std::set<std::string> added{};

static void usage() { die("invalid usage"); }

static void read_dag(const char *dag) {
  auto [_, inserted] = added.insert(dag);
  if (!inserted)
    return;

  sim_sbt obj{};
  sim_sb_copy(&obj, dag);
  sim_sb_path_set_extension(&obj, "o");
  fprintf(out, "%s\n", obj.buffer);

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
    case 'frwk':
      fprintf(out, "-framework\n%s\n", file);
      break;
    case 'libr':
      fprintf(out, "-l%s\n", file);
      break;
    case 'ldir':
      fprintf(out, "-L%s\n", file);
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
  GOPT(opts, argc, argv, "i:");

  const char *input{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    default:
      usage();
    }
  }
  if (opts.argc != 0)
    usage();

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, input);
  target = sim_sb_path_filename(&path);

  sim_sbt args{};
  sim_sb_copy(&args, input);
  sim_sb_path_set_extension(&args, "link");

  if (0 != fopen_s(&out, args.buffer, "wb")) {
    die("could not open argument file: [%s]\n", args.buffer);
  }

  read_dag(input);

  fclose(out);
  return 0;
} catch (...) {
  return 1;
}
