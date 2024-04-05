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
static void to_file(SmallVectorImpl<char> &fname, Token &t) {
  sys::path::remove_filename(fname);
  sys::path::append(fname, to_str(t));
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
class file_list_pragma : public id_list_pragma {
protected:
  using id_list_pragma::id_list_pragma;

  virtual bool process_file(const char * f) = 0;

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    SmallString<256> in{fname};
    to_file(in, t);
    if (!process_file(in.c_str()))
      report(pp, t, "file not found");
  }
};

struct add_dll_pragma : public file_list_pragma, node_holder {
  add_dll_pragma(node *n) : file_list_pragma{"add_dll"}, node_holder{n} {}

  bool process_file(const char * in) override {
    return m_node->add_executable(in);
  }
};

struct add_impl_pragma : public file_list_pragma, node_holder {
  add_impl_pragma(dag::node *n)
      : file_list_pragma{"add_impl"}, node_holder{n} {}

  bool check_ext(auto &f, StringRef ext) {
    sys::path::replace_extension(f, ext);
    return m_node->add_mod_impl(f.c_str());
  }

  bool process_file(const char * in) override {
    SmallString<128> f{in};
    return check_ext(f, "cpp") || check_ext(f, "mm") || check_ext(f, "m");
  }
};

struct add_framework_pragma : public id_list_pragma, node_holder {
  add_framework_pragma(node *n)
      : id_list_pragma{"add_framework"}, node_holder{n} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    notify(pp, t, "added framework");
    m_node->add_framework(to_str(t).str().c_str());
  }
};

struct add_library_pragma : public id_list_pragma, node_holder {
  add_library_pragma(node *n) : id_list_pragma{"add_library"}, node_holder{n} {}

  void process_id(Preprocessor &pp, Token &t, StringRef fname) override {
    notify(pp, t, "added library");
    m_node->add_library(to_str(t).str().c_str());
  }
};

struct add_library_dir_pragma : public file_list_pragma, node_holder {
  add_library_dir_pragma(node *n)
      : file_list_pragma{"add_library_dir"}, node_holder{n} {}

  bool process_file(const char * in) override {
    return m_node->add_library_dir(in);
  }
};

struct add_resource_pragma : public file_list_pragma, node_holder {
  add_resource_pragma(node *n)
      : file_list_pragma{"add_resource"}, node_holder{n} {}

  bool process_file(const char * in) override { return m_node->add_resource(in); }
};

struct add_shader_pragma : public file_list_pragma, node_holder {
  add_shader_pragma(node *n) : file_list_pragma{"add_shader"}, node_holder{n} {}

  bool process_file(const char * in) override { return m_node->add_shader(in); }
};

struct app_pragma : public PragmaHandler, node_holder {
  app_pragma(node *n) : PragmaHandler{"app"}, node_holder{n} {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    m_node->set_app();
  }
};
struct dll_pragma : public PragmaHandler, node_holder {
  dll_pragma(node *n) : PragmaHandler{"dll"}, node_holder{n} {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    m_node->set_dll();
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
  AddPragma(new add_library_pragma(n));
  AddPragma(new add_library_dir_pragma(n));
  AddPragma(new add_resource_pragma(n));
  AddPragma(new add_shader_pragma(n));
  AddPragma(new app_pragma(n));
  AddPragma(new dll_pragma(n));
  AddPragma(new tool_pragma(n));
}

ns_pragma::ns_pragma() : PragmaNamespace{"leco"} {
  AddPragma(new EmptyPragmaHandler());
}
