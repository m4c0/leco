#include "dag.hpp"

#include "../popen/popen.h"
#include "cl.hpp"
#include "context.hpp"
#include "log.hpp"

#include <string.h>

static void stamp(sim_sb *args, char **&argp, const char *arg) {
  sim_sb_concat(args, " ");
  *argp++ = args->buffer + args->len;
  sim_sb_concat(args, arg);
}

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
  char *argv[100]{};
  char **argp = argv;

  sim_sbt args{};
  *argp++ = args.buffer;

  sim_sb_path_copy_parent(&args, leco_argv0);
  sim_sb_path_append(&args, "leco-dagger.exe");
  stamp(&args, argp, "-t");
  stamp(&args, argp, cur_ctx().target.c_str());
  stamp(&args, argp, "-i");
  stamp(&args, argp, n->source());

  if (is_extra_verbose()) {
    stamp(&args, argp, "-d");
  }

  for (auto p = argv + 1; *p && p != argp; p++) {
    (*p)[-1] = 0;
  }

  FILE *f;
  FILE *ferr;
  if (0 != proc_open(argv, &f, &ferr)) {
    return false;
  }

  sim_sbt mod_name{};

  char buf[10240];
  while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
    uint32_t *id = reinterpret_cast<uint32_t *>(buf);
    char *file = reinterpret_cast<char *>(id + 1);
    if (strlen(file) <= 1)
      continue;

    file[strlen(file) - 1] = 0;

    try {
      process_line(n, *id, file);
    } catch (...) {
      return false;
    }
  }

  while (!feof(ferr) && fgets(buf, 1024, ferr) != nullptr) {
    fputs(buf, stderr);
  }

  fclose(f);
  fclose(ferr);

  n->write_to_cache_file();
  return true;
}
