#include "dag.hpp"

#include "../popen/popen.h"
#include "cl.hpp"
#include "clang_dir.hpp"
#include "evoker.hpp"

#include <stdlib.h>

static char *cmp(char *str, const char *prefix) {
  auto len = strlen(prefix);
  if (strncmp(str, prefix, len) != 0)
    return nullptr;
  return str + len;
}

static void log_found(const char *desc, const char *what) {
  if (is_extra_verbose()) {
    fprintf(stderr, "found %s for processing: [%s]\n", desc, what);
  }
}
static bool add_found(const char *desc, const char *what, dag::node *n,
                      bool (dag::node::*fn)(const char *)) {
  log_found(desc, what);
  if (!(n->*fn)(what)) {
    fprintf(stderr, "%s: could not find %s [%s]\n", n->source(), desc, what);
    return false;
  }
  return true;
}
static bool read_file_list(const char *str, dag::node *n,
                           bool (dag::node::*fn)(const char *),
                           const char *desc) {
  while (*str && *str != '\n') {
    while (*str == ' ') {
      str++;
    }
    const char *e{};
    if (*str == '"') {
      str++;
      e = strchr(str, '"');
    } else if (*str && *str != '\n') {
      e = strchr(str, ' ');
      if (e == nullptr) {
        e = strchr(str, '\n');
      }
      if (e == nullptr) {
        e = str + strlen(str);
      }
    }
    if (e == nullptr)
      return false;
    char buf[1024]{};
    strncpy(buf, str, e - str);
    buf[e - str] = 0;
    if (!add_found(desc, buf, n, fn))
      return false;
    str = e + 1;
  }
  return *str == 0 || *str == '\n';
}

static bool add_mod_dep(char *pp, const char *mod, dag::node *n) {
  strchr(pp, ';')[0] = 0;

  sim_sbt mm{};
  if (*pp == ':') {
    sim_sb_copy(&mm, mod);
    if (auto mc = strchr(mm.buffer, ':')) {
      *mc = 0;
      mm.len = strlen(mm.buffer);
    }
    sim_sb_concat(&mm, pp);
  } else {
    sim_sb_copy(&mm, pp);
  }

  return read_file_list(mm.buffer, n, &dag::node::add_mod_dep, "dependency");
}

bool dag::execute(dag::node *n) {
  auto args = evoker{}
                  .push_arg("-E")
                  .push_arg(n->source())
                  .set_cpp_std()
                  .add_predefs()
                  .suppress_pragmas()
                  .prepare_args();
  if (!args)
    return false;

  sim_sbt clang{};
  sim_sb_copy(&clang, clang_exe());

  sim_sbt argfile{};
  sim_sb_copy(&argfile, "@");
  sim_sb_concat(&argfile, args.argument_file());

  FILE *f;
  FILE *ferr;
  char *argv[]{clang.buffer, argfile.buffer, 0};
  if (0 != proc_open(argv, &f, &ferr))
    return false;

  sim_sbt mod_name{};

  char buf[1024];
  while (!feof(f) && fgets(buf, 1024, f) != nullptr) {
    char *p = buf;
    while (*p == ' ') {
      p++;
    }
    if (0 == strcmp(p, "#pragma leco tool\n")) {
      log_found("tool", n->source());
      n->set_tool();
    } else if (cmp(p, "#pragma leco app\n")) {
      log_found("app", n->source());
      n->set_app();
    } else if (cmp(p, "#pragma leco dll\n")) {
      log_found("dll", n->source());
      n->set_dll();
    } else if (auto pp = cmp(p, "#pragma leco add_dll ")) {
      if (!read_file_list(pp, n, &dag::node::add_executable, "dll"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_framework ")) {
      if (!read_file_list(pp, n, &dag::node::add_framework, "framework"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_impl ")) {
      if (!read_file_list(pp, n, &dag::node::add_mod_impl, "impl"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_library ")) {
      if (!read_file_list(pp, n, &dag::node::add_library, "library"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_library_dir ")) {
      if (!read_file_list(pp, n, &dag::node::add_library_dir, "library_dir"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_resource ")) {
      if (!read_file_list(pp, n, &dag::node::add_resource, "resource"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_shader ")) {
      if (!read_file_list(pp, n, &dag::node::add_shader, "shader"))
        return false;
    } else if (cmp(p, "#pragma leco ")) {
      fprintf(stderr, "unknown pragma: %s", p);
      return false;
    } else if (auto pp = cmp(p, "module ")) {
      strchr(pp, ';')[0] = 0;
      sim_sb_copy(&mod_name, pp);
    } else if (auto pp = cmp(p, "export module ")) {
      strchr(pp, ';')[0] = 0;
      sim_sb_copy(&mod_name, pp);
      // TODO: do we need to test and call n->set_main_mod?
    } else if (auto pp = cmp(p, "export import ")) {
      if (!add_mod_dep(pp, mod_name.buffer, n))
        return false;
    } else if (auto pp = cmp(p, "import ")) {
      if (!add_mod_dep(pp, mod_name.buffer, n))
        return false;
    }
  }
  fclose(f);
  fclose(ferr);

  // n->write_to_cache_file();
  return true;
}
