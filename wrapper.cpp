#include "compile.hpp"
#include "context.hpp"
#include "find_deps_action.hpp"
#include "wrapper.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/LexDiagnostic.h"

using namespace clang;
using namespace llvm;

class add_impl_pragma_handler : public PragmaHandler {
  StringRef m_cur_file;

public:
  explicit add_impl_pragma_handler(StringRef f)
      : PragmaHandler{"add_impl"}, m_cur_file{f} {}

  void HandlePragma(Preprocessor &pp, PragmaIntroducer introducer,
                    Token &pragma_tok) {
    Token t;
    do {
      pp.LexUnexpandedToken(t);
      if (t.getKind() == tok::eod) {
        return;
      }
      if (!t.isAnyIdentifier()) {
        pp.Diag(t, diag::err_pp_identifier_arg_not_identifier) << t.getKind();
        return;
      }

      SmallString<128> f{};
      auto dir = sys::path::parent_path(m_cur_file);
      sys::path::append(f, dir, t.getIdentifierInfo()->getName());
      sys::path::replace_extension(f, "cpp");
      compile(f);
    } while (true);
  }
};
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
  pp.AddPragmaHandler("leco", new add_impl_pragma_handler(getCurrentFile()));

  return WrapperFrontendAction::BeginSourceFileAction(ci);
}
