#include "dag.hpp"

#include "../popen/popen.h"
#include "cl.hpp"
#include "context.hpp"
#include "log.hpp"

#include <stdlib.h>
#include <string.h>

static unsigned line{};

static char *cmp(char *str, const char *prefix) {
  auto len = strlen(prefix);
  if (strncmp(str, prefix, len) != 0)
    return nullptr;
  return str + len;
}

static bool add_found(const char *desc, const char *what, dag::node *n,
                      bool (dag::node::*fn)(const char *)) {
  xlog(desc, what);
  if (!(n->*fn)(what)) {
    // Hacking a line number to allow editors automatically jumping to file.
    // TODO: use precompiled definitions to find line numbers
    fprintf(stderr, "%s:%d: could not find %s [%s]\n", n->source(), line, desc,
            what);
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
    str = *e ? e + 1 : e;
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

[[nodiscard]] static bool find_header(dag::node *n, const char *l) {
  auto s = strchr(l, '"');
  if (!l)
    return true;

  s++;

  // <build-in> and <command-line>
  if (*s == '<')
    return true;

  sim_sbt hdr{};
  sim_sb_copy(&hdr, s);

  auto e = strchr(hdr.buffer, '"');
  if (!e)
    return true;

  *e = 0;

  // Flag == 3 means "system header". We don't track them.
  if (strchr(e + 1, '3'))
    return true;

  // Flag == 1 means "entering file".
  if (!strchr(e + 1, '1'))
    return true;

  // Other flags would be "2" meaning "leaving file" and "4" meaning "extern C"
  // block.

  return add_found("header", hdr.buffer, n, &dag::node::add_header);
}

static void stamp(sim_sb *args, char **&argp) {
  sim_sb_concat(args, " ");
  *argp++ = args->buffer + args->len;
}

extern const char *leco_argv0;
bool dag::execute(dag::node *n) {
  char *argv[100]{};
  char **argp = argv;

  sim_sbt args{};
  *argp++ = args.buffer;

  sim_sb_path_copy_parent(&args, leco_argv0);
  sim_sb_path_append(&args, "leco-clang.exe");
  stamp(&args, argp);
  sim_sb_concat(&args, "-t");
  stamp(&args, argp);
  sim_sb_concat(&args, cur_ctx().target.c_str());
  stamp(&args, argp);
  sim_sb_concat(&args, "--");
  stamp(&args, argp);
  sim_sb_concat(&args, "-E");
  stamp(&args, argp);
  sim_sb_concat(&args, n->source());

  for (auto p = argv + 1; *p && p != argp; p++) {
    (*p)[-1] = 0;
  }

  FILE *f;
  FILE *ferr;
  if (0 != proc_open(argv, &f, &ferr))
    return false;

  line = 0;
  sim_sbt mod_name{};

  char buf[1024];
  while (!feof(f) && fgets(buf, 1024, f) != nullptr) {
    char *p = buf;
    while (*p == ' ') {
      p++;
    }
    line++;
    if (0 == strcmp(p, "#pragma leco tool\n")) {
      xlog("tool", n->source());
      n->set_tool();
    } else if (cmp(p, "#pragma leco app\n")) {
      xlog("app", n->source());
      n->set_app();
    } else if (cmp(p, "#pragma leco dll\n")) {
      xlog("dll", n->source());
      n->set_dll();
    } else if (auto pp = cmp(p, "#pragma leco add_build_dep ")) {
      if (!read_file_list(pp, n, &dag::node::add_build_dep, "build dependency"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_dll ")) {
      if (!read_file_list(pp, n, &dag::node::add_executable, "dll"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_framework ")) {
      if (!read_file_list(pp, n, &dag::node::add_framework, "framework"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_impl ")) {
      // TODO: fix add_impl inside parts
      if (!read_file_list(pp, n, &dag::node::add_mod_impl, "impl"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_library ")) {
      if (!read_file_list(pp, n, &dag::node::add_library, "library"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_library_dir ")) {
      if (!read_file_list(pp, n, &dag::node::add_library_dir, "library dir"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_resource ")) {
      if (!read_file_list(pp, n, &dag::node::add_resource, "resource"))
        return false;
    } else if (auto pp = cmp(p, "#pragma leco add_shader ")) {
      if (!read_file_list(pp, n, &dag::node::add_shader, "shader"))
        return false;
    } else if (cmp(p, "#pragma leco ")) {
      fprintf(stderr, "%s:%d: unknown pragma\n", n->source(), line);
      return false;
    } else if (auto pp = cmp(p, "# ")) {
      // # <line> "<file>" <flags>...
      if (!find_header(n, pp))
        return false;

      auto l = atoi(pp);
      if (l != 0)
        line = l - 1;
    } else if (auto pp = cmp(p, "module ")) {
      strchr(pp, ';')[0] = 0;
      sim_sb_copy(&mod_name, pp);
    } else if (auto pp = cmp(p, "export module ")) {
      strchr(pp, ';')[0] = 0;
      sim_sb_copy(&mod_name, pp);
      if (strchr(mod_name.buffer, ':') == nullptr) {
        sim_sbt fn{};
        sim_sb_path_copy_parent(&fn, n->source());
        auto dir = sim_sb_path_filename(&fn);
        if (0 == strcmp(dir, mod_name.buffer))
          n->set_main_mod();
      }
    } else if (auto pp = cmp(p, "export import ")) {
      if (!add_mod_dep(pp, mod_name.buffer, n))
        return false;
    } else if (auto pp = cmp(p, "import ")) {
      if (!add_mod_dep(pp, mod_name.buffer, n))
        return false;
    }
  }

  if (is_extra_verbose()) {
    while (!feof(ferr) && fgets(buf, 1024, ferr) != nullptr) {
      fputs(buf, stderr);
    }
  }

  fclose(f);
  fclose(ferr);

  n->write_to_cache_file();
  return true;
}
