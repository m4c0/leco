#include "dag.hpp"

#include "cl.hpp"
#include "context.hpp"
#include "dag2.hpp"
#include "die.hpp"

#include <string.h>

void prep(sim_sb *cmd, const char *tool);

static void add(const char *desc, dag::node *n,
                bool (dag::node::*fn)(const char *), const char *file) {
  if (!(n->*fn)(file))
    die("could not add %s: [%s]", desc, file);
}

static void process_line(dag::node *n, uint32_t id, const char *file) {
  switch (id) {
  case 'tool':
    n->set_tool();
    break;
  case 'tapp':
    n->set_app();
    break;
  case 'tdll':
    n->set_dll();
    break;
  case 'tmmd':
    n->set_main_mod();
    break;
  case 'bdep':
    add("build dependency", n, &dag::node::add_build_dep, file);
    break;
  case 'dlls':
    add("dll", n, &dag::node::add_executable, file);
    break;
  case 'frwk':
    add("framework", n, &dag::node::add_framework, file);
    break;
  case 'head':
    add("header", n, &dag::node::add_header, file);
    break;
  case 'impl':
    add("implementation", n, &dag::node::add_mod_impl, file);
    break;
  case 'libr':
    add("library", n, &dag::node::add_library, file);
    break;
  case 'ldir':
    add("library directory", n, &dag::node::add_library_dir, file);
    break;
  case 'mdep':
    add("module dependency", n, &dag::node::add_mod_dep, file);
    break;
  case 'rsrc':
    add("resource", n, &dag::node::add_resource, file);
    break;
  case 'shdr':
    add("shader", n, &dag::node::add_shader, file);
    break;
  default:
    die("unknown tag in dag file");
  }
}

void dag::node::create_cache_file() {
  sim_sbt args{10240};
  prep(&args, "leco-dagger.exe");
  sim_sb_concat(&args, " -t ");
  sim_sb_concat(&args, cur_ctx().target.c_str());
  sim_sb_concat(&args, " -i ");
  sim_sb_concat(&args, source());
  sim_sb_concat(&args, " -o ");
  sim_sb_concat(&args, dag());

  if (is_extra_verbose()) {
    sim_sb_concat(&args, " -d");
  }

  try {
    run(args.buffer);
    read_from_cache_file();
  } catch (...) {
    remove(dag());
    throw;
  }
}

void dag::node::read_from_cache_file() {
  dag_read(dag(), [this](auto id, auto file) { process_line(this, id, file); });
}
