#include "bouncer.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "evoker.hpp"
#include "instance.hpp"
#include "link.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace llvm;

namespace {
class tool_pragma_handler : public PragmaHandler {
  bool *m_found_it;

public:
  tool_pragma_handler(bool *f) : PragmaHandler{"tool"}, m_found_it{f} {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    *m_found_it = true;
  }
};
} // namespace

class bouncer : public PreprocessorFrontendAction {
public:
  void ExecuteAction() override {
    auto &ci = getCompilerInstance();
    ci.getPreprocessorOpts().SingleFileParseMode = true;

    bool tool = false;

    auto &pp = ci.getPreprocessor();
    pp.AddPragmaHandler("leco", new tool_pragma_handler(&tool));
    pp.SetSuppressIncludeNotFoundError(true);
    pp.EnterMainSourceFile();

    Token t;
    do {
      pp.Lex(t);
    } while (t.isNot(tok::eof));

    SmallString<128> pwd;
    sys::fs::current_path(pwd); // TODO: check errors
    auto pwd_stem = sys::path::stem(pwd);
    auto file_stem = sys::path::stem(getCurrentFile());
    bool root = pp.isInNamedModule() && pp.getNamedModuleName() == pwd_stem;

    if (!root && !tool)
      return;

    cur_ctx().pcm_reqs.clear();
    if (!compile(getCurrentFile()))
      return;

    if (!tool)
      return;

    cur_ctx().add_pcm_req(getCurrentFile());
    link(getCurrentFile());
  }
};

bool bounce(StringRef path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  if (ext != ".cppm" && ext != ".cpp")
    return true;

  return evoker{}.push_arg("-E").push_arg(path).build().run<bouncer>(false);
}
