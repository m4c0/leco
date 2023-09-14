#include "context.hpp"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;

template <unsigned N>
static bool report(Preprocessor &pp, Token &t, const char (&msg)[N]) {
  auto &d = pp.getDiagnostics();
  auto d_id = d.getCustomDiagID(DiagnosticsEngine::Error, msg);
  d.Report(t.getLocation(), d_id);
  return false;
}

class id_list_pragma : public PragmaHandler {
protected:
  virtual bool process_id(Preprocessor &pp, Token &t, StringRef fname) = 0;

public:
  using PragmaHandler::PragmaHandler;

  void HandlePragma(Preprocessor &pp, PragmaIntroducer introducer,
                    Token &pragma_tok) override {
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

  static bool check_ext(SmallVectorImpl<char> &f, StringRef ext) {
    bool res{};
    sys::path::replace_extension(f, ext);
    sys::fs::is_regular_file(f, res);
    return res;
  }

  bool process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    SmallString<128> f{};
    auto dir = sys::path::parent_path(fname);
    sys::path::append(f, dir, t.getIdentifierInfo()->getName());

    if (!check_ext(f, "cpp") && !check_ext(f, "mm") && !check_ext(f, "m")) {
      return report(pp, t, "module impl not found");
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

struct add_resource_pragma : public PragmaHandler {
  add_resource_pragma() : PragmaHandler{"add_resource"} {}

  bool process_id(Preprocessor &pp, Token &t, StringRef fname) {
    auto lit = StringRef{t.getLiteralData(), t.getLength()}.trim('"');
    bool res{};
    if (sys::fs::is_regular_file(lit, res) && !res) {
      report(pp, t, "resource not found");
      return false;
    }
    cur_ctx().add_pcm_resource(fname, lit);
    return true;
  }
  void HandlePragma(Preprocessor &pp, PragmaIntroducer introducer,
                    Token &pragma_tok) override {
    auto fname = pp.getSourceManager().getFilename(introducer.Loc);

    Token t;
    do {
      pp.LexUnexpandedToken(t);
      if (t.getKind() == tok::eod) {
        return;
      }
      if (!t.isLiteral()) {
        pp.Diag(t, diag::err_pp_identifier_arg_not_identifier) << t.getKind();
        return;
      }

      if (!process_id(pp, t, fname)) {
        return;
      }
    } while (true);
  }
};

struct app_pragma : public PragmaHandler {
  app_pragma() : PragmaHandler{"app"} {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    cur_ctx().exe_type = exe_t::app;
  }
};
struct tool_pragma : public PragmaHandler {
  tool_pragma() : PragmaHandler{"tool"} {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    cur_ctx().exe_type = exe_t::tool;
  }
};

struct ns_pragma : public PragmaNamespace {
  ns_pragma() : PragmaNamespace{"leco"} {
    AddPragma(new add_impl_pragma());
    AddPragma(new add_framework_pragma());
    AddPragma(new add_resource_pragma());
    AddPragma(new app_pragma());
    AddPragma(new tool_pragma());
  }
};
static PragmaHandlerRegistry::Add<ns_pragma> NS{"leco", "leco extensions"};
