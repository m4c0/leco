#include "dag.hpp"

#include "clang_dir.hpp"
#include "diags.hpp"
#include "evoker.hpp"
#include "popen.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"

#include <stdlib.h>

using namespace clang;
using namespace llvm;

namespace {
class ppc : public PPCallbacks {
  DiagnosticsEngine *m_diag;
  dag::node *m_dag;

public:
  explicit ppc(DiagnosticsEngine *diag, dag::node *d)
      : m_diag{diag}
      , m_dag{d} {}

  bool FileNotFound(StringRef filename) override { return true; }
  void moduleImport(SourceLocation loc, ModuleIdPath path,
                    const Module *imp) override {
    assert(path.size() == 1 && "path isn't atomic");
    auto &[id, _] = path[0];
    if (!m_dag->add_mod_dep(id->getName().str().c_str())) {
      diag_error(*m_diag, loc, "module not found");
    }
  }
};

class action : public PreprocessOnlyAction {
  dag::node *m_dag;

public:
  explicit action(dag::node *d) : m_dag{d} {}

  bool BeginSourceFileAction(CompilerInstance &ci) override {
    auto *diag = &ci.getDiagnostics();
    ci.getPreprocessor().addPPCallbacks(std::make_unique<ppc>(diag, m_dag));
    return true;
  }
  void EndSourceFileAction() override {
    SmallString<128> pwd;
    sys::fs::current_path(pwd); // TODO: check errors
    auto pwd_stem = sys::path::stem(pwd);
    auto file_stem = sys::path::stem(getCurrentFile());
    auto file_ext = sys::path::extension(getCurrentFile());
    auto &pp = getCompilerInstance().getPreprocessor();
    bool root = pp.isInNamedModule() && pp.getNamedModuleName() == pwd_stem &&
                file_ext == ".cppm";
    if (root && !m_dag->root())
      m_dag->set_main_mod();
  }
};
} // namespace

static const char *cmp(const char *str, const char *prefix) {
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
  char *argv[]{clang.buffer, argfile.buffer};
  if (0 != proc_open(argv, &f, &ferr))
    return false;

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
    } else if (cmp(p, "export module ")) {
      // printf("%s", buf);
    } else if (cmp(p, "export import ")) {
      // printf("%s", buf);
    } else if (cmp(p, "import ")) {
      // printf("%s", buf);
    }
  }
  fclose(f);
  fclose(ferr);

  auto ci =
      evoker{"-E", n->source(), "dummy"}.set_cpp_std().add_predefs().createCI();

  action a{n};
  if (!ci->ExecuteAction(a))
    return false;

  n->write_to_cache_file();
  return true;
}
