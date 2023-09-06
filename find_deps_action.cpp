#include "find_deps_action.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;

bool compile(StringRef path);

class find_deps_pp_callbacks : public PPCallbacks {
  DiagnosticsEngine *m_diags;
  StringRef m_cur_file;

  void report_compilation_error(SourceLocation loc) {
    auto lvl = DiagnosticsEngine::Error;
    auto did = m_diags->getCustomDiagID(lvl, "failed to compile dependency");
    m_diags->Report(loc, did);
  }
  void report_missing_module(SourceLocation loc) {
    auto lvl = DiagnosticsEngine::Error;
    auto did = m_diags->getCustomDiagID(lvl, "module not found");
    m_diags->Report(loc, did);
  }

  StringRef check_module(Twine t, SmallVectorImpl<char> &s) {
    if (!sys::fs::is_regular_file(t))
      return {};

    return t.toNullTerminatedStringRef(s);
  }

public:
  find_deps_pp_callbacks(DiagnosticsEngine *diags, StringRef file)
      : m_diags{diags}, m_cur_file{file} {}

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

    auto dir = sys::path::parent_path(m_cur_file);

    auto &[id, _] = path[0];
    auto mod_name = id->getName();
    auto p = mod_name.find(":");
    SmallString<128> dep{};
    if (p != StringRef::npos) {
      auto me = mod_name.substr(0, p);
      auto part = mod_name.substr(p + 1);

      if (compile((dir + "/" + me + "-" + part + ".cppm")
                      .toNullTerminatedStringRef(dep)))
        return;

    } else {
      auto t = check_module(dir + "/" + mod_name + ".cppm", dep);
      if (t.empty())
        t = check_module("../" + mod_name + "/" + mod_name + ".cppm", dep);
      if (!sys::fs::is_regular_file(t))
        return report_missing_module(loc);

      if (compile(t))
        return;
    }

    report_missing_module(loc);
    return;
  }
};

void find_deps_action::ExecuteAction() {
  auto *diags = &getCompilerInstance().getDiagnostics();
  auto &pp = getCompilerInstance().getPreprocessor();
  pp.EnterMainSourceFile();
  pp.addPPCallbacks(
      std::make_unique<find_deps_pp_callbacks>(diags, getCurrentFile()));

  Token Tok;
  do {
    pp.Lex(Tok);
  } while (!Tok.is(tok::eof));
}
