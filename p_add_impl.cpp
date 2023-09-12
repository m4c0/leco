#include "compile.hpp"
#include "context.hpp"
#include "diags.hpp"
#include "p_add_impl.hpp"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"

using namespace clang;
using namespace llvm;

static auto missing_impl() {
  auto lvl = DiagnosticsEngine::Error;
  return diags().getCustomDiagID(lvl, "module impl %0 not found");
}

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

    process_id(t.getIdentifierInfo()->getName());
  } while (true);
}

void add_impl_pragma_handler::HandlePragma(Preprocessor &pp,
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

    SmallString<128> f{};
    auto dir = sys::path::parent_path(m_cur_file);
    sys::path::append(f, dir, t.getIdentifierInfo()->getName());
    sys::path::replace_extension(f, "cpp");

    bool res{};
    sys::fs::is_regular_file(f, res); // TODO: check error
    if (!res) {
      pp.Diag(t, missing_impl()) << t.getIdentifierInfo()->getName();
      return;
    }

    cur_ctx().add_pcm_dep(m_cur_file, f);
    if (!compile(f))
      return;
  } while (true);
}

void add_framework_pragma_handler::process_id(StringRef id) {
  cur_ctx().add_pcm_framework(current_file(), id);
}
