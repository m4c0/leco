#pragma leco tool

#define GOPT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "dag2.hpp"
#include "die.hpp"
#include "gopt.hpp"
#include "in2out.hpp"
#include "sim.hpp"

#include <set>
#include <string>

static const char *target;

static void usage() { die("invalid usage"); }

static std::set<std::string> added{};
static void read_dag(const char *dag) {
  auto [_, inserted] = added.insert(dag);
  if (!inserted)
    return;

  dag_read(dag, [](auto id, auto file) {
    switch (id) {
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
  const char *exedir{};

  auto opts = gopt_parse(argc, argv, "e:i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    case 'e':
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

  read_dag(input);
} catch (...) {
  return 1;
}
