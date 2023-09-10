#pragma once
#include "clang/Lex/Pragma.h"

class add_impl_pragma_handler : public clang::PragmaHandler {
  llvm::StringRef m_cur_file;

public:
  explicit add_impl_pragma_handler(llvm::StringRef f)
      : PragmaHandler{"add_impl"}, m_cur_file{f} {}

  void HandlePragma(clang::Preprocessor &pp, clang::PragmaIntroducer introducer,
                    clang::Token &pragma_tok);
};
