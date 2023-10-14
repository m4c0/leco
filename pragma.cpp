#include "dag.hpp"
#include "diags.hpp"
#include "pragma.hpp"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;
using namespace dag;

template <unsigned N>
static auto report(Preprocessor &pp, Token &t, const char (&msg)[N]) {
  return diag_error(pp.getDiagnostics(), t.getLocation(), msg);
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

class node_holder {
protected:
  node *m_node;

public:
  node_holder(node *n) : m_node{n} {}
};

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

struct add_dll_pragma : public id_list_pragma, node_holder {
  add_dll_pragma(dag::node *n) : id_list_pragma("add_dll"), node_holder{n} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    SmallString<256> in{fname};
    llvm::sys::path::remove_filename(in);
    llvm::sys::path::append(in, to_str(t));

    bool res{};
    if (sys::fs::is_regular_file(in, res) && !res) {
      report(pp, t, "library not found");
    } else {
      notify(pp, t, "added library to bundle");
      m_node->add_executable(in);
    }
  }
};
struct add_impl_pragma : public id_list_pragma, node_holder {
  add_impl_pragma(dag::node *n) : id_list_pragma{"add_impl"}, node_holder{n} {}

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
      report(pp, t, "module impl not found");
      return;
    }

    notify(pp, t, "added implementation");
    m_node->add_mod_impl(f);
  }
};

struct add_framework_pragma : public id_list_pragma, node_holder {
  add_framework_pragma(node *n)
      : id_list_pragma{"add_framework"}, node_holder{n} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    notify(pp, t, "added framework");
    m_node->add_framework(to_str(t));
  }
};

struct add_include_dir_pragma : public id_list_pragma {
  add_include_dir_pragma() : id_list_pragma{"add_include_dir"} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    notify(pp, t, "added include directory");

    SmallString<256> in{fname};
    llvm::sys::path::remove_filename(in);
    llvm::sys::path::append(in, to_str(t));

    auto de = pp.getFileManager().getDirectoryRef(in);
    if (auto err = de.takeError()) {
      report(pp, t, "error adding include directory: %0")
          << toString(std::move(err));
      return;
    }
    DirectoryLookup dl{*de, SrcMgr::C_User, false};
    pp.getHeaderSearchInfo().AddSearchPath(dl, false);
    pp.getHeaderSearchInfo().AddSearchPath(dl, true);
  }
};

struct add_library_pragma : public id_list_pragma, node_holder {
  add_library_pragma(node *n) : id_list_pragma{"add_library"}, node_holder{n} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    notify(pp, t, "added library");
    m_node->add_library(to_str(t));
  }
};

struct add_resource_pragma : public id_list_pragma, node_holder {
  add_resource_pragma(node *n)
      : id_list_pragma{"add_resource"}, node_holder{n} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) {
    auto lit = to_str(t);
    bool res{};
    if (sys::fs::is_regular_file(lit, res) && !res) {
      report(pp, t, "resource not found");
    } else {
      m_node->add_resource(lit);
    }
  }
};

struct add_shader_pragma : public id_list_pragma, node_holder {
  add_shader_pragma(node *n) : id_list_pragma{"add_shader"}, node_holder{n} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) {
    auto lit = to_str(t);
    bool res{};
    if (sys::fs::is_regular_file(lit, res) && !res) {
      report(pp, t, "shader not found");
    } else {
      m_node->add_shader(lit);
    }
  }
};

struct app_pragma : public PragmaHandler, node_holder {
  app_pragma(node *n) : PragmaHandler{"app"}, node_holder{n} {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    m_node->set_app();
  }
};
struct tool_pragma : public PragmaHandler, node_holder {
  tool_pragma(node *n) : PragmaHandler{"tool"}, node_holder{n} {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    m_node->set_tool();
  }
};

ns_pragma::ns_pragma(dag::node *n) : PragmaNamespace{"leco"} {
  AddPragma(new add_dll_pragma(n));
  AddPragma(new add_framework_pragma(n));
  AddPragma(new add_impl_pragma(n));
  AddPragma(new add_include_dir_pragma());
  AddPragma(new add_library_pragma(n));
  AddPragma(new add_resource_pragma(n));
  AddPragma(new add_shader_pragma(n));
  AddPragma(new app_pragma(n));
  AddPragma(new tool_pragma(n));
}

ns_pragma::ns_pragma() : PragmaNamespace{"leco"} {
  AddPragma(new EmptyPragmaHandler());
}
static PragmaHandlerRegistry::Add<ns_pragma> NS{"leco", "leco extensions"};
