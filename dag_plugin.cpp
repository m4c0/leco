#include "dag.hpp"

#include "cl.hpp"
#include "context.hpp"
#include "dag2.hpp"
#include "die.hpp"

#include <string.h>

void prep(sim_sb *cmd, const char *tool);

void dag::node::create_cache_file() {
  sim_sbt args{10240};
  prep(&args, "leco-dagger.exe");
  sim_sb_concat(&args, " -t ");
  sim_sb_concat(&args, cur_ctx().target.c_str());
  sim_sb_concat(&args, " -i ");
  sim_sb_concat(&args, source());
  sim_sb_concat(&args, " -o ");
  sim_sb_concat(&args, dag());

  run(args.buffer);
  read_from_cache_file();
}

void dag::node::read_from_cache_file() {
  dag_read(dag(), [this](auto id, auto file) {
    switch (id) {
    case 'tool':
      set_tool();
      break;
    case 'tapp':
      set_app();
      break;
    case 'tdll':
      set_dll();
      break;
    case 'tmmd':
      set_main_mod();
      break;
    case 'bdep':
      add_build_dep(file);
      break;
    case 'head':
      add_header(file);
      break;
    case 'impl':
      add_mod_impl(file);
      break;
    case 'mdep':
      add_mod_dep(file);
      break;
    }
  });
}
