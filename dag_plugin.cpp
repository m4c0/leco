#include "dag.hpp"

#include "diags.hpp"
#include "evoker.hpp"
#include "pragma.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"

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
    ci.getPreprocessor().AddPragmaHandler(new ns_pragma(m_dag));
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

bool dag::execute(dag::node *n) {
  auto ci =
      evoker{"-E", n->source(), "dummy"}.set_cpp_std().add_predefs().createCI();

  action a{n};
  if (!ci->ExecuteAction(a))
    return false;

  n->write_to_cache_file();
  return true;
}
