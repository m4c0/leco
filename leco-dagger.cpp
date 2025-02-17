#pragma leco tool

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

import gopt;
import mtime;
import popen;
import pprent;
import sim;
import strset;
import sys;

enum class exe_t {
  none,
  main_mod,
  dll,
  tool,
  app,
};

static const sim::sb dag_file_version { "2025-01-04" };

static bool dump_errors {};
static bool verbose {};
static sim::sb source {};
static FILE * out{stdout};
static const char * out_filename {};
static const char * target { sys::host_target };

static unsigned line {};
static exe_t exe_type {};
static sim::sb mod_name {};

static void usage() {
  sys::die(R"(
LECO tool responsible for preprocessing C++ files containing leco pragmas and
storing dependencies in a DAG-like file.

Usage: ../leco/leco.exe dagger [-d] [-i <input.cpp>] [-t <target>] [-v]

Where:
        -d: Dump errors from clang if enabled

        -i: Source file name. Required if not recursing.

        -t: Target triple. Defaults to host target.

        -v: Verbose. Output name of inspected files.

)");
}

[[noreturn]] static void error(const char *msg) {
  sys::die("%s:%d: %s\n", *source, line, msg);
}
[[noreturn]] static void missing_file(const char *desc) {
  sys::die("%s:%d: could not find %s\n", *source, line, desc);
}

static void output(uint32_t code, const char *msg) {
  fwrite(&code, sizeof(uint32_t), 1, out);
  fputs(msg, out);
  fputc('\n', out);
}

static const char *cmp(const char *str, const char *prefix) {
  auto len = strlen(prefix);
  if (strncmp(str, prefix, len) != 0) return nullptr;
  return str + len;
}
static const char *chomp(const char *str, const char *prefix) {
  static sim::sb buf {};

  auto ptr = cmp(str, prefix);
  if (!ptr) return ptr;

  buf = sim::sb { ptr };

  auto scptr = strchr(*buf, ';');
  if (!scptr) return scptr;
  *scptr = 0;

  return *buf;
}

static void stamp(sim::sb * args, char **& argp, const char * arg) {
  *args += " ";
  *argp++ = **args + args->len;
  *args += arg;
}

static bool print_if_found(const char *rel_path, const char *desc,
                           uint32_t code) {
  sim::sb abs = sim::path_parent(*source) / rel_path;
  if (mtime::of(*abs) != 0) {
    abs = sim::path_real(*abs);
  } else {
    abs = sim::path_real(rel_path);
    if (mtime::of(*abs) == 0) return false;
  }

  output(code, *abs);
  return true;
}
static void print_found(const char *rel_path, const char *desc, uint32_t code) {
  if (print_if_found(rel_path, desc, code)) return;

  missing_file(desc);
}
static void print_asis(const char *rel_path, const char *desc, uint32_t code) {
  output(code, rel_path);
}
static bool print_dag_if_found(const char *src, const char *desc, uint32_t code,
                               uint32_t dag_code) {
  if (!print_if_found(src, desc, code)) return false;

  auto dag = sim::path_parent(*sim::path_real(src)) / "out" / target / sim::path_filename(src);
  dag.path_extension("dag");
  output(dag_code, *dag);

  // TODO: merge dags from deps (also recursing?)
  return true;
}

using printer_t = void (*)(const char *, const char *, uint32_t);
static void read_file_list(const char *str, const char *desc, uint32_t code,
                           printer_t prfn = print_found) {
  while (*str && *str != '\n') {
    while (*str == ' ') str++;

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
    if (e == nullptr) throw 1;

    auto buf = sim::sb { str };
    (*buf)[e - str] = 0;

    prfn(*buf, desc, code);

    str = *e ? e + 1 : e;
  }
  if (*str != 0 && *str != '\n') throw 1;
}

static void add_xcfw(const char * str, const char * desc, uint32_t code) {
  auto path = sim::sb { str };

  if (sys::is_tgt_iphoneos(target))     path /= "ios-arm64";
  else if (sys::is_tgt_ios_sim(target)) path /= "ios-arm64_x86_64-simulator";
  else if (sys::is_tgt_osx(target))     path /= "macos-arm64_x86_64";
  else sys::die("xcframework is only supported in apple platforms");

  path /= sim::path_filename(str);
  path.path_extension("framework");

  print_found(*path, desc, code);
}

static void add_shdr(const char * src, const char * desc, uint32_t code) {
  print_found(src, desc, code);

  auto out = sim::path_parent(*sim::path_real(src)) / "out" / target / sim::path_filename(src) + ".spv";
  output('rsrc', *out);
}

static void find_header(const char *l) {
  auto s = strchr(l, '"');
  if (!s) error("mismatching quote");

  s++;

  // <build-in> and <command-line>
  if (*s == '<') return;

  auto hdr = sim::sb { s };

  // Ignore system headers
  auto e = strchr(*hdr, '"');
  if (!e) return;

  *e = 0;

  // Flag == 3 means "system header". We don't track them.
  if (strchr(e + 1, '3'))
    return;

  // Flag == 1 means "entering file".
  if (!strchr(e + 1, '1'))
    return;

  // Other flags would be "2" meaning "leaving file" and "4" meaning "extern C"
  // block.

  print_found(*hdr, "header", 'head');
}
static void add_mod_dep(const char *p, const char *desc) {
  sim::sb mm {};
  if (*p == ':') {
    mm = { mod_name };
    if (auto mc = strchr(*mm, ':')) {
      *mc = 0;
      mm.len = strlen(*mm);
    }
    mm += p;
  } else {
    mm = sim::sb { p };
  }

  sim::sb srcdir = sim::path_parent(*source);
  sim::sb pp { *mm };

  // Module parts
  auto sc = strchr(*pp, ':');
  if (sc != nullptr) {
    *sc = '-';

    auto dep = srcdir / *pp + ".cppm";
    if (print_dag_if_found(*dep, desc, 'mdep', 'mdag')) return;
  }

  // Module in the same folder
  auto dep = srcdir / *mm + ".cppm";
  if (print_dag_if_found(*dep, desc, 'mdep', 'mdag')) return;

  // Module in sibling folder
  dep = sim::path_parent(*srcdir) / *mm / *mm + ".cppm";
  if (print_dag_if_found(*dep, desc, 'mdep', 'mdag')) return;

  // Module in sibling folder with "-" instead of "_"
  char * u;
  while ((u = strchr(*pp, '_')) != nullptr) *u = '-';
  dep = sim::path_parent(*srcdir) / *pp / *mm + ".cppm";
  if (print_dag_if_found(*dep, desc, 'mdep', 'mdag')) return;

  missing_file(desc);
}

static bool check_extension(sim::sb * mi, const char *desc, const char *ext) {
  mi->path_extension(ext);
  return print_dag_if_found(**mi, desc, 'impl', 'idag');
}

static void add_impl(const char *mod_impl, const char *desc, uint32_t code) {
  auto mi = sim::path_parent(*source) / mod_impl;

  if (check_extension(&mi, desc, "cpp") || check_extension(&mi, desc, "c") ||
      check_extension(&mi, desc, "mm") || check_extension(&mi, desc, "m"))
    return;

  missing_file(desc);
}

static void output_file_tags() {
  auto path = sim::path_parent(*source) / "out" / target / source.path_filename();

  path.path_extension("o");
  output('objf', *path);

  if (sim::path_extension(*source) == ".cppm") {
    path.path_extension("pcm");
    output('pcmf', *path);
  }
}

static void output_root_tag() {
  auto path = sim::path_parent(*source) / "out" / target / source.path_filename();

  switch (exe_type) {
  case exe_t::none:
    break;
  case exe_t::main_mod:
    output('tmmd', "");
    break;
  case exe_t::app:
    path.path_extension("exe");
    output('tapp', *path);
    break;
  case exe_t::dll:
    if (sys::is_tgt_windows(target)) path.path_extension("dll");
    else if (sys::is_tgt_apple(target)) path.path_extension("dylib");
    else path.path_extension("so");

    output('tdll', *path);
    break;
  case exe_t::tool:
    if (sys::is_tgt_host(target)) {
      path.path_extension("exe");
      output('tool', *path);
    }
    break;
  }
}

static bool exe_pragma(const char * p, const char * e, exe_t t) {
  p = cmp(p, e);
  if (!p) return false;
  if (*p != '\n' && *p != '\r') error("expecting newline after this pragma");
  if (exe_type != exe_t::none) error("multiple executable type found");
  exe_type = t;
  return true;
}
static bool add_pragma(const char * p, const char * id, uint32_t code, printer_t prfn = print_found) {
  p = cmp(p, "add_");
  if (!p) return false;
  p = cmp(p, id);
  if (!p) return false;
  p = cmp(p, " ");
  if (!p) return false;

  read_file_list(p, id, code, prfn);
  return true;
}
static const char * bundle_cmp(const char * p, const char * id) {
  p = cmp(p, id);
  if (!p) return nullptr;
  p = cmp(p, " ");
  if (!p) return nullptr;

  if (exe_type != exe_t::app) error("display name is only supported for apps");

  return p;
}
static bool prop_pragma(const char * p, const char * id, uint32_t code) {
  p = bundle_cmp(p, id);
  if (!p) return false;
  read_file_list(p, id, code, print_asis);
  return true;
}
static bool flag_pragma(const char * p, const char * id, uint32_t code) {
  p = bundle_cmp(p, id);
  if (!p) return false;
  if (!cmp(p, "\n")) return false;
  output(code, "");
  return true;
}
static bool pragma(const char * p) {
  p = cmp(p, "#pragma leco ");
  if (!p) return false;

  if (exe_pragma(p, "tool", exe_t::tool)) return true;
  if (exe_pragma(p, "app",  exe_t::app))  return true;
  if (exe_pragma(p, "dll",  exe_t::dll))  return true;

  if (add_pragma(p, "dll",         'dlls'))             return true;
  if (add_pragma(p, "embed",       'embd'))             return true;
  if (add_pragma(p, "framework",   'frwk', print_asis)) return true;
  if (add_pragma(p, "impl",        'impl', add_impl))   return true;
  if (add_pragma(p, "include_dir", 'idir'))             return true;
  if (add_pragma(p, "library",     'libr', print_asis)) return true;
  if (add_pragma(p, "library_dir", 'ldir'))             return true;
  if (add_pragma(p, "resource",    'rsrc'))             return true;
  if (add_pragma(p, "static_lib",  'slib'))             return true;
  if (add_pragma(p, "shader",      'shdr', add_shdr))   return true;
  if (add_pragma(p, "xcframework", 'xcfw', add_xcfw))   return true;

  if (prop_pragma(p, "display_name", 'name')) return true;
  if (prop_pragma(p, "app_id",       'apid')) return true;
  if (prop_pragma(p, "app_version",  'apvr')) return true;

  if (flag_pragma(p, "portrait",  'port')) return true;
  if (flag_pragma(p, "landscape", 'land')) return true;

  error("unknown pragma");
}

void run() {
  if (verbose) sys::log("inspecting", *source);

  char *clang_argv[100]{};
  char **argp = clang_argv;

  auto args = sys::tool_cmd("clang");
  *argp++ = *args;

  stamp(&args, argp, "-t");
  stamp(&args, argp, target);
  stamp(&args, argp, "--");
  stamp(&args, argp, "-E");
  stamp(&args, argp, *source);

  for (auto p = clang_argv + 1; *p && p != argp; p++) {
    (*p)[-1] = 0;
  }

  p::proc proc{clang_argv};

  line = 0;
  exe_type = {};
  mod_name = {};

  while (proc.gets()) {
    const char *p = proc.last_line_read();
    while (*p == ' ') p++;
    line++;

    if (pragma(p)) continue;

    if (auto pp = cmp(p, "# ")) {
      // # <line> "<file>" <flags>...
      find_header(pp);

      auto l = atoi(pp);
      if (l != 0)
        line = l - 1;
    }
        
    else if (auto pp = chomp(p, "module ")) {
      if (pp[0] < 'a' || pp[0] > 'z') {
        // Ignoring if not identifier
      } else if (0 != strcmp(pp, ":private")) {
        mod_name = sim::sb { pp };
        add_mod_dep(pp, "main module dependency");
      }
    } else if (auto pp = chomp(p, "export module ")) {
      mod_name = sim::sb { pp };
      if (strchr(*mod_name, ':') == nullptr) {
        auto fn = sim::path_parent(*source);
        auto dir = fn.path_filename();
        if (mod_name == dir && exe_type == exe_t::none)
          exe_type = exe_t::main_mod;
      }
    } else if (auto pp = chomp(p, "export import ")) {
      add_mod_dep(pp, "exported dependency");
    } else if (auto pp = chomp(p, "import ")) {
      add_mod_dep(pp, "dependency");
    }
  }

  if (dump_errors) while (proc.gets_err()) fputs(proc.last_line_read(), stderr);

  output('srcf', *source);
  // TODO: output mod_name

  output_root_tag();
  output_file_tags();

  fclose(out);
}

static void check_and_run(const char * src, const char * dag) {
  if (mtime::of(dag) > mtime::of(src)) {
    bool must_run = true;
    sys::dag_read(dag, [&](auto id, auto file) {
      if (id != 'vers') return;
      if (dag_file_version == file) must_run = false;
    });
    if (!must_run) return;
  }

  source = sim::sb { src };
  out_filename = dag;
  sys::mkdirs(*sim::path_parent(dag));
  out = sys::fopen(out_filename, "w");
  output('vers', *dag_file_version);
  run();
}

static str::set done {};
static void process(const char * path) {
  if (!done.insert(path)) return;

  auto dag = sim::path_parent(path) / "out" / target / sim::path_filename(path);
  dag.path_extension("dag");
  check_and_run(path, *dag);

  sys::dag_read(*dag, [](auto id, auto file) {
    switch (id) {
      case 'impl':
      case 'mdep': process(file); break;
    }
  });
}

int main(int argc, char **argv) try {
  auto opts = gopt_parse(argc, argv, "dvt:", [&](auto ch, auto val) {
    switch (ch) {
      case 'd': dump_errors = true; break;
      case 't': target = val; break;
      case 'v': verbose = true; break;
      default: usage();
    }
  });
  if (opts.argc != 0) usage();

  for (auto file : pprent::list(".")) {
    auto ext = sim::path_extension(file);
    if (!ext.len) continue;

    if (ext != ".cppm" && ext != ".cpp" && ext != ".c") continue;

    process(*sim::path_real(file));
  }
  return 0;
} catch (...) {
  if (out != nullptr) fclose(out);
  if (out_filename != nullptr) remove(out_filename);
  return 1;
}
