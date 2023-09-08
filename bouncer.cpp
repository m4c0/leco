#include "bouncer.hpp"
#include "compile.hpp"
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

bool bounce(StringRef path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  if (ext == ".cppm") {
    return (stem.find("-") == StringRef::npos) ? compile(path) : true;
  }

  if (ext != ".cpp")
    return true;

  auto b = evoker{}.push_arg("-c").set_inout(path, ".o").build().run<bouncer>();
  if (b && b->is_valid())
    return compile(path); // TODO: link as well

  return true;
}
