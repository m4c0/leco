#pragma once
#include "clang/Lex/Pragma.h"

class id_list_pragma_handler : public clang::PragmaHandler {
  llvm::StringRef m_cur_file;

protected:
  virtual void process_id(llvm::StringRef id) = 0;

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
class add_impl_pragma_handler : public clang::PragmaHandler {
  llvm::StringRef m_cur_file;

public:
  explicit add_impl_pragma_handler(llvm::StringRef f)
      : PragmaHandler{"add_impl"}, m_cur_file{f} {}

  void HandlePragma(clang::Preprocessor &pp, clang::PragmaIntroducer introducer,
                    clang::Token &pragma_tok);
};

class add_framework_pragma_handler : public id_list_pragma_handler {
protected:
  void process_id(llvm::StringRef id) override;

public:
  explicit add_framework_pragma_handler(llvm::StringRef f)
      : id_list_pragma_handler{"add_framework", f} {}
};
