#pragma leco tool
#define POPEN_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../popen/popen.h"
#include "die.hpp"
#include "fopen.hpp"
#include "mkdir.h"
#include "sim.hpp"
#include "targets.hpp"

#include <stdint.h>

import gopt;
import mtime;

enum class exe_t {
  none,
  main_mod,
  dll,
  tool,
  app,
};

static const char *argv0;
static sim_sbt source{};
static unsigned line{};
static exe_t exe_type{};
static sim_sbt mod_name{};
static FILE *out{stdout};
static const char *out_filename{};
static const char *target{HOST_TARGET};

static void usage() {
  die(R"(
LECO tool responsible for preprocessing C++ files containing leco pragmas and
storing dependencies in a DAG-like file.

Usage: %s [-d] -i <input.cpp> [-o <output.dag>] [-t <target>]

Where:
        -d: Dump errors from clang if enabled

        -i: Source file name

        -o: Output file name. Defaults to standard output.

        -t: Target triple. Defaults to host target.

)",
      argv0);
}

static void error(const char *msg) {
  die("%s:%d: %s\n", source.buffer, line, msg);
}

static void output(uint32_t code, const char *msg) {
  fwrite(&code, sizeof(uint32_t), 1, out);
  fputs(msg, out);
  fputc('\n', out);
}

static char *cmp(char *str, const char *prefix) {
  auto len = strlen(prefix);
  if (strncmp(str, prefix, len) != 0)
    return nullptr;
  return str + len;
}

static void stamp(sim_sb *args, char **&argp, const char *arg) {
  sim_sb_concat(args, " ");
  *argp++ = args->buffer + args->len;
  sim_sb_concat(args, arg);
}

static void set_exe_type(exe_t t) {
  if (exe_type != exe_t::none)
    error("multiple executable type found");
  exe_type = t;
}

static bool print_if_found(const char *rel_path, const char *desc,
                           uint32_t code) {
  sim_sbt path{};
  sim_sb_path_copy_parent(&path, source.buffer);
  sim_sb_path_append(&path, rel_path);

  sim_sbt abs{};
  if (mtime::of(path.buffer) == 0) {
    sim_sb_path_copy_real(&abs, rel_path);
    if (mtime::of(abs.buffer) == 0) {
      return false;
    }
  } else {
    sim_sb_path_copy_real(&abs, path.buffer);
  }

  output(code, abs.buffer);
  return true;
}
static void print_found(const char *rel_path, const char *desc, uint32_t code) {
  if (print_if_found(rel_path, desc, code))
    return;

  die("%s:%d: could not find %s\n", source.buffer, line, desc);
}
static void print_asis(const char *rel_path, const char *desc, uint32_t code) {
  output(code, rel_path);
}

using printer_t = void (*)(const char *, const char *, uint32_t);
static void read_file_list(const char *str, const char *desc, uint32_t code,
                           printer_t prfn = print_found) {
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
      throw 1;

    sim_sbt buf{};
    sim_sb_copy(&buf, str);
    buf.buffer[e - str] = 0;

    prfn(buf.buffer, desc, code);

    str = *e ? e + 1 : e;
  }
  if (*str != 0 && *str != '\n')
    throw 1;
}

static void find_header(const char *l) {
  auto s = strchr(l, '"');
  if (!s)
    error("mismatching quote");

  s++;

  // <build-in> and <command-line>
  if (*s == '<')
    return;

  sim_sbt hdr{};
  sim_sb_copy(&hdr, s);

  // Ignore system headers
  auto e = strchr(hdr.buffer, '"');
  if (!e)
    return;

  *e = 0;

  // Flag == 3 means "system header". We don't track them.
  if (strchr(e + 1, '3'))
    return;

  // Flag == 1 means "entering file".
  if (!strchr(e + 1, '1'))
    return;

  // Other flags would be "2" meaning "leaving file" and "4" meaning "extern C"
  // block.

  print_found(hdr.buffer, "header", 'head');
}
static bool print_mod_dep(const char *src, const char *desc) {
  if (!print_if_found(src, desc, 'mdep'))
    return false;

  sim_sbt dag{};
  sim_sb_path_copy_real(&dag, src);
  sim_sb_path_parent(&dag);
  sim_sb_path_append(&dag, "out");
  sim_sb_path_append(&dag, target);
  sim_sb_path_append(&dag, sim_path_filename(src));
  sim_sb_path_set_extension(&dag, "dag");
  output('mdag', dag.buffer);

  // TODO: merge dags from deps (also recursing?)
  return true;
}
static void add_mod_dep(char *p, const char *desc) {
  sim_sbt mm{};
  if (*p == ':') {
    sim_sb_copy(&mm, mod_name.buffer);
    if (auto mc = strchr(mm.buffer, ':')) {
      *mc = 0;
      mm.len = strlen(mm.buffer);
    }
    sim_sb_concat(&mm, p);
  } else {
    sim_sb_copy(&mm, p);
  }

  sim_sbt pp{};
  sim_sb_copy(&pp, mm.buffer);

  // Module parts
  auto sc = strchr(pp.buffer, ':');
  if (sc != nullptr) {
    *sc = '-';

    sim_sbt dep{};
    sim_sb_path_copy_parent(&dep, source.buffer);
    sim_sb_path_append(&dep, pp.buffer);
    sim_sb_concat(&dep, ".cppm");
    if (print_mod_dep(dep.buffer, desc))
      return;
  }

  // Module in the same folder
  sim_sbt dep{};
  sim_sb_path_copy_parent(&dep, source.buffer);
  sim_sb_path_append(&dep, mm.buffer);
  sim_sb_concat(&dep, ".cppm");
  if (print_mod_dep(dep.buffer, desc))
    return;

  // Module in sibling folder
  sim_sb_path_parent(&dep);
  sim_sb_path_parent(&dep);
  sim_sb_path_append(&dep, mm.buffer);
  sim_sb_path_append(&dep, mm.buffer);
  sim_sb_concat(&dep, ".cppm");
  if (print_mod_dep(dep.buffer, desc))
    return;

  // Module in sibling folder with "-" instead of "_"
  while ((p = strchr(pp.buffer, '_')) != nullptr) {
    *p = '-';
  }
  sim_sb_path_parent(&dep);
  sim_sb_path_parent(&dep);
  sim_sb_path_append(&dep, pp.buffer);
  sim_sb_path_append(&dep, mm.buffer);
  sim_sb_concat(&dep, ".cppm");
  if (print_mod_dep(dep.buffer, desc))
    return;

  die("%s:%d: could not find %s\n", source.buffer, line, desc);
}

static bool print_mod_impl(const char *src, const char *desc) {
  if (!print_if_found(src, desc, 'impl'))
    return false;

  sim_sbt dag{};
  sim_sb_path_copy_real(&dag, src);
  sim_sb_path_parent(&dag);
  sim_sb_path_append(&dag, "out");
  sim_sb_path_append(&dag, target);
  sim_sb_path_append(&dag, sim_path_filename(src));
  sim_sb_path_set_extension(&dag, "dag");
  output('idag', dag.buffer);

  // TODO: merge dags from deps (also recursing?)
  return true;
}
static void add_impl(const char *mod_impl, const char *desc, uint32_t code) {
  sim_sbt mi{};
  sim_sb_path_copy_parent(&mi, source.buffer);
  sim_sb_path_append(&mi, mod_impl);

  sim_sb_path_set_extension(&mi, "cpp");
  if (print_mod_impl(mi.buffer, desc))
    return;

  sim_sb_path_set_extension(&mi, "c");
  if (print_mod_impl(mi.buffer, desc))
    return;

  sim_sb_path_set_extension(&mi, "mm");
  if (print_mod_impl(mi.buffer, desc))
    return;

  sim_sb_path_set_extension(&mi, "m");
  if (print_mod_impl(mi.buffer, desc))
    return;

  die("%s:%d: could not find %s\n", source.buffer, line, desc);
}

void run(int argc, char **argv) {
  bool dump_errors{};
  f::open fout{};

  argv0 = argv[0];

  auto opts = gopt_parse(argc, argv, "do:i:t:", [&](auto ch, auto val) {
    switch (ch) {
    case 'd':
      dump_errors = true;
      break;
    case 'i':
      sim_sb_path_copy_real(&source, val);
      break;
    case 'o': {
      sim_sbt parent{};
      sim_sb_path_copy_parent(&parent, val);
      mkdirs(parent.buffer);
      fout = f::open{val, "w"};
      out = *fout;
      out_filename = val;
      break;
    }
    case 't':
      target = val;
      break;
    default:
      usage();
    }
  });

  if (source.len == 0 || opts.argc != 0)
    usage();

  char *clang_argv[100]{};
  char **argp = clang_argv;

  sim_sbt args{};
  *argp++ = args.buffer;

  sim_sb_path_copy_parent(&args, argv[0]);
  sim_sb_path_append(&args, "leco-clang.exe");
  stamp(&args, argp, "-t");
  stamp(&args, argp, target);
  stamp(&args, argp, "--");
  stamp(&args, argp, "-E");
  stamp(&args, argp, source.buffer);

  for (auto p = clang_argv + 1; *p && p != argp; p++) {
    (*p)[-1] = 0;
  }

  FILE *f;
  FILE *ferr;
  if (0 != proc_open(clang_argv, &f, &ferr))
    throw 1;

  line = 0;

  char buf[1024];
  while (!feof(f) && fgets(buf, 1024, f) != nullptr) {
    char *p = buf;
    while (*p == ' ') {
      p++;
    }
    line++;
    if (0 == strcmp(p, "#pragma leco tool\n")) {
      set_exe_type(exe_t::tool);
    } else if (cmp(p, "#pragma leco app\n")) {
      set_exe_type(exe_t::app);
    } else if (cmp(p, "#pragma leco dll\n")) {
      set_exe_type(exe_t::dll);
    } else if (auto pp = cmp(p, "#pragma leco add_build_dep ")) {
      read_file_list(pp, "build dependency", 'bdep');
    } else if (auto pp = cmp(p, "#pragma leco add_dll ")) {
      read_file_list(pp, "dll", 'dlls');
    } else if (auto pp = cmp(p, "#pragma leco add_framework ")) {
      read_file_list(pp, "framework", 'frwk', print_asis);
    } else if (auto pp = cmp(p, "#pragma leco add_impl ")) {
      read_file_list(pp, "implementation", 'impl', add_impl);
    } else if (auto pp = cmp(p, "#pragma leco add_library ")) {
      read_file_list(pp, "library", 'libr', print_asis);
    } else if (auto pp = cmp(p, "#pragma leco add_library_dir ")) {
      read_file_list(pp, "library dir", 'ldir');
    } else if (auto pp = cmp(p, "#pragma leco add_resource ")) {
      read_file_list(pp, "resource", 'rsrc');
    } else if (auto pp = cmp(p, "#pragma leco add_shader ")) {
      read_file_list(pp, "shader", 'shdr');
    } else if (cmp(p, "#pragma leco ")) {
      error("unknown pragma");
    } else if (auto pp = cmp(p, "# ")) {
      // # <line> "<file>" <flags>...
      find_header(pp);

      auto l = atoi(pp);
      if (l != 0)
        line = l - 1;
    } else if (auto pp = cmp(p, "module ")) {
      strchr(pp, ';')[0] = 0;
      if (0 != strcmp(pp, ":private")) {
        sim_sb_copy(&mod_name, pp);
        add_mod_dep(pp, "main module dependency");
      }
    } else if (auto pp = cmp(p, "export module ")) {
      strchr(pp, ';')[0] = 0;
      sim_sb_copy(&mod_name, pp);
      if (strchr(mod_name.buffer, ':') == nullptr) {
        sim_sbt fn{};
        sim_sb_path_copy_parent(&fn, source.buffer);
        auto dir = sim_sb_path_filename(&fn);
        if (0 == strcmp(dir, mod_name.buffer) && exe_type == exe_t::none)
          exe_type = exe_t::main_mod;
      }
    } else if (auto pp = cmp(p, "export import ")) {
      strchr(p, ';')[0] = 0;
      add_mod_dep(pp, "exported dependency");
    } else if (auto pp = cmp(p, "import ")) {
      strchr(p, ';')[0] = 0;
      add_mod_dep(pp, "dependency");
    }
  }

  if (dump_errors) {
    while (!feof(ferr) && fgets(buf, 1024, ferr) != nullptr) {
      fputs(buf, stderr);
    }
  }

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, source.buffer);
  sim_sb_path_append(&path, "out");
  sim_sb_path_append(&path, target);
  sim_sb_path_append(&path, sim_sb_path_filename(&source));

  switch (exe_type) {
  case exe_t::none:
    break;
  case exe_t::main_mod:
    output('tmmd', "");
    break;
  case exe_t::app:
    sim_sb_path_set_extension(&path, "exe");
    output('tapp', path.buffer);
    break;
  case exe_t::dll:
    if (IS_TGT(target, TGT_WINDOWS)) {
      sim_sb_path_set_extension(&path, "dll");
    } else if (IS_TGT_APPLE(target)) {
      sim_sb_path_set_extension(&path, "dylib");
    } else {
      sim_sb_path_set_extension(&path, "so");
    }
    output('tdll', path.buffer);
    break;
  case exe_t::tool:
    if (0 == strcmp(target, HOST_TARGET)) {
      sim_sb_path_set_extension(&path, "exe");
      output('tool', path.buffer);
    }
    break;
  }

  fclose(f);
  fclose(ferr);
}

int main(int argc, char **argv) try {
  run(argc, argv);
  return 0;
} catch (...) {
  if (out_filename != nullptr) {
    remove(out_filename);
  }
  return 1;
}
