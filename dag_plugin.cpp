#include "dag.hpp"

#include "cl.hpp"
#include "context.hpp"
#include "log.hpp"

#include <string.h>

static void add(dag::node *n, bool (dag::node::*fn)(const char *),
                const char *file) {
  if (!(n->*fn)(file)) {
    throw 0;
  }
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
    add(n, &dag::node::add_build_dep, file);
    break;
  case 'dlls':
    add(n, &dag::node::add_executable, file);
    break;
  case 'frwk':
    add(n, &dag::node::add_framework, file);
    break;
  case 'head':
    add(n, &dag::node::add_header, file);
    break;
  case 'impl':
    add(n, &dag::node::add_mod_impl, file);
    break;
  case 'libr':
    add(n, &dag::node::add_library, file);
    break;
  case 'ldir':
    add(n, &dag::node::add_library_dir, file);
    break;
  case 'mdep':
    add(n, &dag::node::add_mod_dep, file);
    break;
  case 'rsrc':
    add(n, &dag::node::add_resource, file);
    break;
  case 'shdr':
    add(n, &dag::node::add_shader, file);
    break;
  default:
    throw 0;
  }
}

extern const char *leco_argv0;
bool dag::execute(dag::node *n) {
  sim_sbt args{10240};
  sim_sb_path_copy_parent(&args, leco_argv0);
  sim_sb_path_append(&args, "leco-dagger.exe");
  sim_sb_concat(&args, " -t ");
  sim_sb_concat(&args, cur_ctx().target.c_str());
  sim_sb_concat(&args, " -i ");
  sim_sb_concat(&args, n->source());
  sim_sb_concat(&args, " -o ");
  sim_sb_concat(&args, n->dag());

  if (is_extra_verbose()) {
    sim_sb_concat(&args, " -d");
  }

  if (0 != system(args.buffer)) {
    remove(n->dag());
    return false;
  }
  return n->read_from_cache_file();
}

bool dag::node::read_from_cache_file() {
  FILE *f = fopen(dag(), "r");
  if (!f) {
    fprintf(stderr, "dag file not found: [%s]\n", dag());
    return false;
  }

  char buf[10240];
  while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
    if (strlen(buf) < 5) {
      puts("invalid line");
      return false;
    }

    uint32_t *id = reinterpret_cast<uint32_t *>(buf);
    char *file = reinterpret_cast<char *>(id + 1);
    file[strlen(file) - 1] = 0;

    try {
      process_line(this, *id, file);
    } catch (...) {
      fprintf(stderr, "file not found: [%s]\n", file);
      return false;
    }
  }

  fclose(f);

  return true;
}
