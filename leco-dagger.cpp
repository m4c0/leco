#pragma leco tool

import c42;
import sys;

using namespace c;

enum class exe_t {
  none,
  main_mod,
  dll,
  tool,
  app,
  test,
};

static const char * dag_file_version = "2025-10-19";

static sim::sb source {};
static sys::file * current_output;
static auto target = sys::target();

static unsigned line {};
static exe_t exe_type {};
static sim::sb mod_name {};

static void usage() {
  die(R"(
LECO tool responsible for preprocessing C++ files containing leco pragmas and
storing dependencies in a DAG-like file.
)");
}

static_assert(sizeof(unsigned) == 4);

[[noreturn]] static void error(const char *msg) {
  dief("%s:%d: %s\n", *source, line, msg);
}
[[noreturn]] static void missing_file(const char *desc) {
  dief("%s:%d: could not find %s\n", *source, line, desc);
}

static void output(unsigned code, const char *msg) {
  fputfn(*current_output, "%.4s%s", reinterpret_cast<char *>(&code), msg);
}

static const char *cmp(const char *str, const char *prefix) {
  if (!sv::unsafe(str).starts_with(sv::unsafe(prefix))) return nullptr;
  auto len = strlen(prefix);
  return str + len;
}
static const char * cmp(const char * str, auto ... prefixes) {
  const auto c = [&](const char * p) { return str = cmp(str, p); };
  if ((... && c(prefixes))) return str;
  return nullptr;
}

static sim::sb path_of(const char * rel_path) {
  sim::sb abs = sim::path_parent(*source) / rel_path;
  abs = mtime::of(*abs) ? sim::path_real(*abs) : sim::path_real(rel_path);
  return mtime::of(*abs) ? abs : sim::sb {};
}
static bool print_if_found(const char *rel_path, const char *desc, unsigned code) {
  auto abs = path_of(rel_path);
  if (abs.len == 0) return false;

  output(code, *abs);
  return true;
}
static void print_found(const char *rel_path, const char *desc, unsigned code) {
  if (print_if_found(rel_path, desc, code)) return;

  missing_file(desc);
}
static void print_asis(const char *rel_path, const char *desc, unsigned code) {
  output(code, rel_path);
}
static bool print_dag_if_found(const char *src, const char *desc, unsigned code, unsigned dag_code) {
  if (!print_if_found(src, desc, code)) return false;

  auto dag = sim::path_parent(*sim::path_real(src)) / "out" / target / sim::path_filename(src);
  dag.path_extension("dag");
  output(dag_code, *dag);

  // TODO: merge dags from deps (also recursing?)
  return true;
}
static void print_dag_found(const char *src, const char *desc, unsigned code, unsigned dag_code) {
  if (print_dag_if_found(src, desc, code, dag_code)) return;

  missing_file(desc);
}

using printer_t = void (*)(const char *, const char *, unsigned);
static void read_file_list(const char *str, const char *desc, unsigned code,
                           printer_t prfn = print_found) {
  while (*str && *str != '\n') {
    while (*str == ' ') str++;

    const char *e{};
    if (*str == '"') {
      str++;
      e = strchr(str, '"');
    } else if (*str && *str != '\n') {
      e = strchr(str, ' ');
      if (e == nullptr) e = strchr(str, '\n');
      if (e == nullptr) e = str + strlen(str);
    }
    if (e == nullptr) throw 1;

    auto buf = sim::sb { str };
    (*buf)[e - str] = 0;

    prfn(*buf, desc, code);

    str = *e ? e + 1 : e;
  }
  if (*str != 0 && *str != '\n') throw 1;
}

static void add_xcfw(const char * str, const char * desc, unsigned code) {
  auto path = sim::sb { str };

  if (sys::is_tgt_iphoneos())     path /= "ios-arm64";
  else if (sys::is_tgt_ios_sim()) path /= "ios-arm64_x86_64-simulator";
  else if (sys::is_tgt_osx())     path /= "macos-arm64_x86_64";
  else die("xcframework is only supported in apple platforms");

  path /= sim::path_filename(str);
  path.path_extension("framework");

  print_found(*path, desc, code);
}

static void add_shdr(const char * src, const char * desc, unsigned code) {
  print_found(src, desc, 'shdr');

  auto ext = sys::is_tgt_wasm() ? ".gles" : ".spv";

  auto out = sim::path_parent(*path_of(src)) / "out" / target / sim::path_filename(src) + ext;
  output(code, *out);
}

static void add_import(sv name, sv part) {
  auto srcdir = sim::path_parent(*source);

  // Module part
  if (part != "") {
    auto dep = srcdir / *mod_name + "-" + part + ".cppm";
    print_dag_found(*dep, "imported part", 'mdep', 'mdag');
    return;
  }

  // Module in the same folder
  auto dep = srcdir / name + ".cppm";
  if (print_dag_if_found(*dep, "imported module", 'mdep', 'mdag')) return;

  // Module in sibling folder
  dep = sim::path_parent(*srcdir) / name / name + ".cppm";
  if (print_dag_if_found(*dep, "imported module", 'mdep', 'mdag')) return;

  missing_file("imported module");
}

static bool check_extension(sim::sb * mi, const char *desc, const char *ext) {
  mi->path_extension(ext);
  return print_dag_if_found(**mi, desc, 'impl', 'idag');
}

static void add_impl(const char *mod_impl, const char *desc, unsigned code) {
  auto mi = sim::path_parent(*source) / mod_impl;

  if (check_extension(&mi, desc, "cpp")) return;
  if (check_extension(&mi, desc, "c"))   return;
  if (check_extension(&mi, desc, "mm"))  return;
  if (check_extension(&mi, desc, "m"))   return;

  missing_file(desc);
}

static auto exe_path(const sim::sb & src) {
  auto path = sim::path_parent(*source) / "out" / target / src.path_filename();
  path.path_extension("exe");
  return path;
}
static auto dll_path(const sim::sb & src) {
  auto path = sim::path_parent(*source) / "out" / target / src.path_filename();
  if      (sys::is_tgt_windows()) path.path_extension("dll");
  else if (sys::is_tgt_apple())   path.path_extension("dylib");
  else                            path.path_extension("so");
  return path;
}

static void add_plgn(const char *mod_impl, const char *desc, unsigned code) {
  // TODO: check if target is a DLL
  auto src = path_of(mod_impl);
  if (src.len == 0) return missing_file(desc);
  output('plgn', *dll_path(src));
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
  switch (exe_type) {
  case exe_t::none:
    break;
  case exe_t::main_mod:
    output('tmmd', "");
    break;
  case exe_t::app: {
    auto path = exe_path(source);
    output('tapp', *path);

    if (sys::is_tgt_osx()) {
      path.path_extension("app");
      output('edir', *(path / "Contents/MacOS"));
      output('fdir', *(path / "Contents/Frameworks"));
      output('rdir', *(path / "Contents/Resources"));
    } else if (sys::is_tgt_iphoneos()) {
      path.path_parent();
      path /= "export.xcarchive/Products/Applications";
      path /= source.path_filename();
      path.path_extension("app");
      output('edir', *path);
      output('fdir', *(path / "Frameworks"));
      output('rdir', *path);
    } else {
      path.path_extension("app");
      output('edir', *path);
      output('rdir', *path);
    }

    break;
  }
  case exe_t::dll:
    output('tdll', *dll_path(source));
    break;
  case exe_t::test:
    if (sys::is_tgt_host()) {
      auto path = exe_path(source);
      output('test', *path);
    }
    break;
  case exe_t::tool:
    if (sys::is_tgt_host()) {
      auto path = exe_path(source);
      output('tool', *path);

      path.path_parent();
      output('edir', *path);
    }
    break;
  }
}

static bool exe_pragma(const char * p, const char * e, exe_t t) {
  p = cmp(p, e);
  if (!p) return false;
  if (*p) error("expecting newline after this pragma");
  if (exe_type != exe_t::none) error("multiple executable type found");
  exe_type = t;
  return true;
}
static bool add_pragma(const char * p, const char * id, unsigned code, printer_t prfn = print_found) {
  if (!(p = cmp(p, "add_", id, " "))) return false;
  read_file_list(p, id, code, prfn);
  return true;
}
static bool prop_pragma(const char * p, const char * id, unsigned code) {
  if (!(p = cmp(p, id, " "))) return false;
  read_file_list(p, id, code, print_asis);
  return true;
}
static bool flag_pragma(const char * p, const char * id, unsigned code) {
  if (!(p = cmp(id, "\n"))) return false;
  output(code, "");
  return true;
}
static void check_app() {
  if (exe_type != exe_t::app) error("this pragma is only supported for apps");
}
static void pragma(const char * p) {
  if (add_pragma(p, "dll",          'dlls'))             return;
  if (add_pragma(p, "framework",    'frwk', print_asis)) return;
  if (add_pragma(p, "impl",         'impl', add_impl))   return;
  if (add_pragma(p, "include_dir",  'idir'))             return;
  if (add_pragma(p, "library",      'libr', print_asis)) return;
  if (add_pragma(p, "library_dir",  'ldir'))             return;
  if (add_pragma(p, "plugin",       'plgn', add_plgn))   return;
  if (add_pragma(p, "resource",     'rsrc'))             return;
  if (add_pragma(p, "resource_dir", 'rdir'))             return;
  if (add_pragma(p, "rpath",        'rpth'))             return;
  if (add_pragma(p, "static_lib",   'slib'))             return;
  if (add_pragma(p, "shader",       'rsrc', add_shdr))   return;
  if (add_pragma(p, "xcframework",  'xcfw', add_xcfw))   return;

  if (prop_pragma(p, "display_name", 'name')) return check_app();
  if (prop_pragma(p, "app_id",       'apid')) return check_app();
  if (prop_pragma(p, "app_version",  'apvr')) return check_app();

  if (flag_pragma(p, "portrait",  'port')) return check_app();
  if (flag_pragma(p, "landscape", 'land')) return check_app();

  if (exe_pragma(p, "test", exe_t::test)) return;
  if (exe_pragma(p, "tool", exe_t::tool)) return;
  if (exe_pragma(p, "app",  exe_t::app))  return;
  if (exe_pragma(p, "dll",  exe_t::dll))  return;

  sim::sb buf { "unknown pragma: " };
  auto pp = strchr(p, ' ');
  buf.printf("%.*s", pp - p, p);

  error(*buf);
}

static bool run_with_c42(const char * src) {
  struct defs : c42::defines {
    bool has(sv name) const override {
      bool res = false;
      sys::for_each_target_def([&](sv env) {
        res |= env == name;
      });
      return res;
    }
  } d {};
  sys::file f { src, "rb" };
  fseek(f, 0, seek_end);
  unsigned len = ftell(f);
  fseek(f, 0, seek_set);

  sim::sb buf { len };
  fread(*buf, len, 1, f);

  auto ctx = c42::preprocess(&d, sv { *buf, len });
  bool exporting = false;
  bool erred = false;
  for (auto it = ctx.begin(); it != ctx.end(); it++) {
    auto t = *it;
    line = t.line;
    switch (t.type) {
      case c42::t_pragma: {
        auto [ns, r] = ctx.txt(t).split(' ');
        if (ns != "leco") continue;

        pragma(*(sim::sb {} + r));
        break;
      }
      case c42::t_include: {
        auto hdr = ctx.txt(t);
        if (hdr[0] == '<') continue;
        errln(hdr);
        // print_found(*hdr, "header", 'head');
        break;
      }
      case c42::t_error: {
        errln(src, ":", t.line, ":", t.column, ": ", ctx.txt(t));
        erred = true;
        break;
      }
      case c42::t_export: {
        exporting = true;
        continue;
      }
      case c42::t_module: {
        auto name = ctx.txt(t); 
        if (name == "") continue;

        auto part = it[1].type == c42::t_ex ? ctx.txt(it[1]) : "";
        if (part == "private") continue;

        mod_name = sim::printf("%.*s", name.size(), name.begin());
        if (!exporting) {
          auto dep = sim::path_parent(src) / *mod_name + ".cppm";
          print_dag_found(*dep, "main module dependency", 'mdep', 'mdag');
        } else if (part == "") {
          auto fn = sim::path_parent(src);
          auto dir = fn.path_filename();
          if (mod_name == dir && exe_type == exe_t::none) {
            exe_type = exe_t::main_mod;
          }
        }
        break;
      }
      case c42::t_import: {
        auto name = ctx.txt(t);
        auto part = it[1].type == c42::t_ex ? ctx.txt(it[1]) : "";
        add_import(name, part);
        break;
      }
      default:
        break;
    }
    exporting = false;
  }

  return !erred;
}

enum run_result { OK, ERR, SKIPPED };
[[nodiscard]] static run_result run(const char * dag, const char * src, bool roots_only) try {
  auto ext = sim::path_extension(src);
  auto mode = "c++";
  if      (ext == ".c" ) mode = "c";
  else if (ext == ".m" ) mode = "objective-c";
  else if (ext == ".mm") mode = "objective-c++";

  p::proc proc { *sys::tool_cmd("clang"), "-x", mode, src, "-E" };
  sys::file f { dag, "w" };

  source = sim::sb { src };
  current_output = &f;
  line = 0;
  exe_type = {};
  mod_name = {};

  if (!run_with_c42(src)) return run_result::ERR;

  if (roots_only && exe_type == exe_t::none) return run_result::SKIPPED;

  output('vers', dag_file_version);
  output('srcf', *source);
  // TODO: output mod_name

  output_root_tag();
  output_file_tags();
  return run_result::OK;
} catch (...) {
  return run_result::ERR;
}

static void create_gitignore(const auto & out) {
  auto path = out / ".gitignore";
  if (mtime::of(*path) > 0) return;

  fputln(sys::file { *path, "w" }, "*");
}

static void check_and_run(const char * src, bool roots_only) {
  auto parent = sim::path_parent(src) / "out" / target;
  sys::mkdirs(*parent);
  create_gitignore(sim::path_parent(*parent));

  auto dag = (parent / sim::path_filename(src)).path_extension("dag");

  if (mtime::of(*dag) > mtime::of(src)) {
    if (dag_file_version == sys::read_dag_tag('vers', *dag)) return;
  }

  switch (run(*dag, src, roots_only)) {
    case run_result::OK:
      return;
    case run_result::ERR:
      remove(*dag);
      throw 0;
    case run_result::SKIPPED:
      remove(*dag);
      return;
  }
}

static void check_non_root(const char * _, unsigned id, const char * file) {
  if (id != 'mdep' && id != 'impl') return;
  return check_and_run(file, false);
}

int main(int argc, char **argv) try {
  if (argc != 1) usage();

  for (auto file : pprent::list(".")) {
    auto ext = sim::path_extension(file);
    if (!ext.len) continue;

    if (ext != ".cppm" && ext != ".cpp" && ext != ".c" && ext != ".m" && ext != ".mm") continue;

    check_and_run(*sim::path_real(file), true);
  }

  sys::for_each_dag(true, &check_non_root);
  return 0;
} catch (...) {
  return 1;
}
