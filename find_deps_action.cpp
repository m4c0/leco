#include "find_deps_action.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;

bool compile(StringRef path);

class find_deps_pp_callbacks : public PPCallbacks {
  DiagnosticsEngine *m_diags;

public:
  find_deps_pp_callbacks(DiagnosticsEngine *diags) : m_diags{diags} {}

  void EnteredSubmodule(Module *m, SourceLocation loc,
                        bool for_pragma) override {
    assert(false && "missing support for submodule");
  }

  void LeftSubmodule(Module *m, SourceLocation loc, bool for_pragma) override {
    assert(false && "missing support for submodule");
  }

  void moduleImport(SourceLocation loc, ModuleIdPath path,
                    const Module *imported) override {
    assert(path.size() == 1 && "path isn't atomic");

    auto &[id, _] = path[0];
    auto mod_name = id->getName();
    auto p = mod_name.find(":");
    if (p != StringRef::npos) {
      auto me = mod_name.substr(0, p);
      auto part = mod_name.substr(p + 1);

      SmallString<128> dep{};
      if (compile((me + "-" + part + ".cppm").toStringRef(dep)))
        return;
    } else {
      if (compile(mod_name))
        return;
    }

    auto lvl = DiagnosticsEngine::Error;
    auto did = m_diags->getCustomDiagID(lvl, "failed to compile dependency");
    m_diags->Report(loc, did);
  }
};

void find_deps_action::ExecuteAction() {
  auto *diags = &getCompilerInstance().getDiagnostics();
  auto &pp = getCompilerInstance().getPreprocessor();
  pp.addPPCallbacks(std::make_unique<find_deps_pp_callbacks>(diags));
  pp.EnterMainSourceFile();

  Token Tok;
  do {
    pp.Lex(Tok);
  } while (!Tok.is(tok::eof));
}
