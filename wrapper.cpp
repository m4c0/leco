#include "context.hpp"
#include "find_deps_action.hpp"
#include "wrapper.hpp"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

class tool_pragma_handler : public PragmaHandler {
public:
  tool_pragma_handler() : PragmaHandler{"tool"} {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    cur_ctx().tool = true;
  }
};

bool wrapper_action::BeginSourceFileAction(CompilerInstance &ci) {
  auto *diags = &ci.getDiagnostics();
  auto &pp = ci.getPreprocessor();

  pp.addPPCallbacks(
      std::make_unique<find_deps_pp_callbacks>(diags, getCurrentFile()));

  pp.AddPragmaHandler("leco", new tool_pragma_handler());

  return WrapperFrontendAction::BeginSourceFileAction(ci);
}
