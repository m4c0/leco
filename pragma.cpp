#include "context.hpp"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;

class id_list_pragma : public PragmaHandler {
protected:
  virtual bool process_id(Preprocessor &pp, Token &t, StringRef fname) = 0;

public:
  using PragmaHandler::PragmaHandler;

  void HandlePragma(Preprocessor &pp, PragmaIntroducer introducer,
                    Token &pragma_tok) {
    auto fname = pp.getSourceManager().getFilename(introducer.Loc);

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

      if (!process_id(pp, t, fname)) {
        return;
      }
    } while (true);
  }
};

struct add_impl_pragma : public id_list_pragma {
  add_impl_pragma() : id_list_pragma{"add_impl"} {}

  bool process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    SmallString<128> f{};
    auto dir = sys::path::parent_path(fname);
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

    cur_ctx().add_pcm_dep(fname, f);
    cur_ctx().add_pending(f);
    return true;
  }
};

struct add_framework_pragma : public id_list_pragma {
  add_framework_pragma() : id_list_pragma{"add_framework"} {}

  bool process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    cur_ctx().add_pcm_framework(fname, t.getIdentifierInfo()->getName());
    return true;
  }
};

struct ns_pragma : public PragmaNamespace {
  ns_pragma() : PragmaNamespace{"leco"} {
    AddPragma(new add_impl_pragma());
    AddPragma(new add_framework_pragma());
  }
};
static PragmaHandlerRegistry::Add<ns_pragma> NS{"leco", "leco extensions"};
