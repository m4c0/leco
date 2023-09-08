#include "bouncer.hpp"
#include "evoker.hpp"
#include "instance.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;

class bouncer : public PreprocessorFrontendAction {
  bool m_valid{};

public:
  void ExecuteAction() override {
    auto *diags = &getCompilerInstance().getDiagnostics();
    auto &pp = getCompilerInstance().getPreprocessor();
    pp.EnterMainSourceFile();

    Token Tok;
    do {
      pp.Lex(Tok);
    } while (!Tok.is(tok::eof));

    m_valid = !pp.isInNamedModule();
  }

  [[nodiscard]] bool is_valid() { return m_valid; }
};

bool is_valid_root_compilation(StringRef path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  if (ext == ".cppm")
    return stem.find("-") == StringRef::npos;

  if (ext != ".cpp")
    return false;

  auto b = evoker{}.push_arg("-c").set_inout(path, ".o").build().run<bouncer>();
  return b && b->is_valid();
}
