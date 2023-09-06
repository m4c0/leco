#include "find_deps_action.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;

bool compile(StringRef path);

class find_deps_pp_callbacks : public PPCallbacks {
public:
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
    errs() << "dep: " << mod_name << "\n";
    if (p != StringRef::npos) {
      auto me = mod_name.substr(0, p);
      auto part = mod_name.substr(p + 1);

      SmallString<128> dep{};
      errs() << compile((me + "-" + part + ".cppm").toStringRef(dep)) << "\n";
    } else {
      errs() << compile(mod_name) << "\n";
    }
  }
};

void find_deps_action::ExecuteAction() {
  auto &pp = getCompilerInstance().getPreprocessor();
  pp.addPPCallbacks(std::make_unique<find_deps_pp_callbacks>());
  pp.EnterMainSourceFile();

  Token Tok;
  do {
    pp.Lex(Tok);
  } while (!Tok.is(tok::eof));
}
