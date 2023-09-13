#include "bouncer.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "evoker.hpp"
#include "instance.hpp"
#include "link.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
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

static bool compile_pending() {
  auto &pend = cur_ctx().pending_compilation;
  while (!pend.empty()) {
    auto f = pend.extract(pend.begin()).value();
    if (!compile(f))
      return false;
  }
  return true;
}

class bouncer : public PreprocessOnlyAction {
  bool tool;

public:
  bool BeginSourceFileAction(CompilerInstance &ci) override {
    auto &pp = ci.getPreprocessor();
    pp.AddPragmaHandler("leco", new tool_pragma_handler(&tool));
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

    tool &= cur_ctx().native_target;

    if (!root && !tool)
      return;

    cur_ctx().pcm_reqs.clear();
    if (!compile(getCurrentFile()))
      return;

    if (!compile_pending())
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

  auto ci = evoker{}.push_arg("-E").push_arg(path).createCI();
  ci->getPreprocessorOpts().SingleFileParseMode = true;

  bouncer b{};
  return ci->ExecuteAction(b);
}
