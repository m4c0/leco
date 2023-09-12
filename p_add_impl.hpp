#pragma once
#include "clang/Lex/Pragma.h"

class id_list_pragma_handler : public clang::PragmaHandler {
  llvm::StringRef m_cur_file;

protected:
  virtual bool process_id(clang::Preprocessor &pp, clang::Token &t) = 0;

  [[nodiscard]] constexpr auto current_file() const noexcept {
    return m_cur_file;
  }

  explicit id_list_pragma_handler(llvm::StringRef name,
                                  llvm::StringRef cur_file)
      : PragmaHandler{name}, m_cur_file{cur_file} {}

public:
  void HandlePragma(clang::Preprocessor &pp, clang::PragmaIntroducer introducer,
                    clang::Token &pragma_tok);
};
class add_impl_pragma_handler : public id_list_pragma_handler {
protected:
  bool process_id(clang::Preprocessor &pp, clang::Token &t) override;

public:
  explicit add_impl_pragma_handler(llvm::StringRef f)
      : id_list_pragma_handler{"add_impl", f} {}
};

class add_framework_pragma_handler : public id_list_pragma_handler {
protected:
  bool process_id(clang::Preprocessor &pp, clang::Token &t) override;

public:
  explicit add_framework_pragma_handler(llvm::StringRef f)
      : id_list_pragma_handler{"add_framework", f} {}
};
