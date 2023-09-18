#include "context.hpp"
#include "diags.hpp"
#include "in2out.hpp"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;

template <unsigned N>
static void report(Preprocessor &pp, Token &t, const char (&msg)[N]) {
  diag_error(pp.getDiagnostics(), t.getLocation(), msg);
}
template <unsigned N>
static void notify(Preprocessor &pp, Token &t, const char (&msg)[N]) {
  diag_remark(pp.getDiagnostics(), t.getLocation(), msg);
}
static StringRef to_str(Token &t) {
  StringRef txt{};
  if (t.isAnyIdentifier()) {
    txt = t.getIdentifierInfo()->getName();
  } else if (t.isLiteral()) {
    txt = StringRef{t.getLiteralData(), t.getLength()}.trim('"');
  }
  return txt;
}

class id_list_pragma : public PragmaHandler {
protected:
  virtual void process_id(Preprocessor &pp, Token &t, StringRef fname) = 0;

public:
  using PragmaHandler::PragmaHandler;

  void HandlePragma(Preprocessor &pp, PragmaIntroducer introducer,
                    Token &pragma_tok) override {
    auto fname = pp.getSourceManager().getFilename(introducer.Loc);

    Token t;
    do {
      pp.Lex(t);
      if (t.getKind() == tok::eod) {
        return;
      }

      if (to_str(t) == "") {
        report(pp, t, "expecting identifier or string");
      } else {
        process_id(pp, t, fname);
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

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    SmallString<128> f{};
    auto dir = sys::path::parent_path(fname);
    sys::path::append(f, dir, to_str(t));

    if (!check_ext(f, "cpp") && !check_ext(f, "mm") && !check_ext(f, "m")) {
      return report(pp, t, "module impl not found");
    }

    notify(pp, t, "queueing implementation");
    cur_ctx().add_pcm_dep(fname, f);
    cur_ctx().add_pending(f);
  }
};

struct add_framework_pragma : public id_list_pragma {
  add_framework_pragma() : id_list_pragma{"add_framework"} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    notify(pp, t, "added framework");
    cur_ctx().add_pcm_framework(fname, to_str(t));
  }
};

struct add_library_pragma : public id_list_pragma {
  add_library_pragma() : id_list_pragma{"add_library"} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    notify(pp, t, "added library");
    SmallString<128> lib{"-l"};
    lib.append(to_str(t));
    cur_ctx().add_pcm_library(fname, lib);
  }
};

struct add_object_pragma : public id_list_pragma {
  add_object_pragma() : id_list_pragma{"add_object"} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    auto lit = to_str(t);
    bool res{};
    if (sys::fs::is_regular_file(lit, res) && !res) {
      report(pp, t, "object not found");
    } else {
      notify(pp, t, "added as library");
      cur_ctx().add_pcm_library(fname, lit);
    }
  }
};

struct add_resource_pragma : public id_list_pragma {
  add_resource_pragma() : id_list_pragma{"add_resource"} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) {
    auto lit = to_str(t);
    bool res{};
    if (sys::fs::is_regular_file(lit, res) && !res) {
      report(pp, t, "resource not found");
    } else {
      cur_ctx().add_pcm_resource(fname, lit);
    }
  }
};

struct add_shader_pragma : public id_list_pragma {
  add_shader_pragma() : id_list_pragma{"add_shader"} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) {
    notify(pp, t, "adding shader");

    SmallString<256> out{};
    auto lit = to_str(t);
    in2out(lit, out);
    out.append(".spv");

    llvm::sys::fs::create_directories(llvm::sys::path::parent_path(out));

    auto cmd = ("glslangValidator -V -o " + out + " " + lit).str();
    if (0 == system(cmd.c_str())) {
      cur_ctx().add_pcm_resource(fname, out);
    } else {
      report(pp, t, "failed to compile shader");
    }
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
    AddPragma(new add_library_pragma());
    AddPragma(new add_object_pragma());
    AddPragma(new add_resource_pragma());
    AddPragma(new add_shader_pragma());
    AddPragma(new app_pragma());
    AddPragma(new tool_pragma());
  }
};
static PragmaHandlerRegistry::Add<ns_pragma> NS{"leco", "leco extensions"};
