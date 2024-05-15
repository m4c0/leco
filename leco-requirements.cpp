#pragma leco tool
#define GOPT_IMPLEMENTATION
#define PPRENT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../pprent/pprent.hpp"
#include "dag2.hpp"
#include "die.hpp"
#include "fopen.hpp"
#include "gopt.hpp"
#include "host_target.hpp"
#include "sim.hpp"

#include <set>
#include <string>

static const char *target{HOST_TARGET};

static std::set<std::string> collected{};

void collect_deps(sim_sb *path) {
  auto [_, x] = collected.emplace(path->buffer);
  if (!x)
    return;

  sim_sb_path_append(path, "out");
  sim_sb_path_append(path, target);

  for (auto entry : pprent::list(path->buffer)) {
    if (0 != strcmp(".dag", sim_path_extension(entry)))
      continue;

    sim_sb_path_append(path, entry);

    dag_read(path->buffer, [](auto id, auto file) {
      switch (id) {
      case 'impl':
      case 'mdep': {
        sim_sbt p{};
        sim_sb_path_copy_parent(&p, file);
        collect_deps(&p);
        break;
      }
      default:
        break;
      }
    });

    sim_sb_path_parent(path);
  }
}

static void usage() { die("invalid usage"); }

int main(int argc, char ** argv) {
  auto opts = gopt_parse(argc, argv, "t:", [&](auto ch, auto val) {
    switch (ch) {
    case 't':
      target = val;
      break;
    default:
      usage();
    }
  });
  if (opts.argc != 0)
    usage();

  sim_sbt cwd{};
  sim_sb_path_copy_real(&cwd, ".");
  collect_deps(&cwd);

  for (const auto &s : collected) {
    puts(sim_path_filename(s.c_str()));
  }
}
