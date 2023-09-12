#include "compile.hpp"
#include "context.hpp"
#include "diags.hpp"
#include "p_add_impl.hpp"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"

using namespace clang;
using namespace llvm;

void id_list_pragma_handler::HandlePragma(Preprocessor &pp,
                                          PragmaIntroducer introducer,
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

    if (!process_id(pp, t)) {
      return;
    }
  } while (true);
}

bool add_impl_pragma_handler::process_id(Preprocessor &pp, Token &t) {
  SmallString<128> f{};
  auto dir = sys::path::parent_path(current_file());
  sys::path::append(f, dir, t.getIdentifierInfo()->getName());
  sys::path::replace_extension(f, "cpp");

  bool res{};
  sys::fs::is_regular_file(f, res); // TODO: check error
  if (!res) {
    auto &d = pp.getDiagnostics();
    auto d_id =
        d.getCustomDiagID(DiagnosticsEngine::Error, "module impl not found");
    d.Report(t.getLocation(), d_id);
    return false;
  }

  cur_ctx().add_pcm_dep(current_file(), f);
  if (!compile(f)) {
    auto &d = pp.getDiagnostics();
    auto d_id = d.getCustomDiagID(DiagnosticsEngine::Error,
                                  "module impl failed to compile");
    d.Report(t.getLocation(), d_id);
    return false;
  }
  return true;
}

bool add_framework_pragma_handler::process_id(Preprocessor &pp, Token &t) {
  cur_ctx().add_pcm_framework(current_file(), t.getIdentifierInfo()->getName());
  return true;
}
