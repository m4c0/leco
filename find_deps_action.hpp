#pragma once

#include "clang/Frontend/FrontendActions.h"

class find_deps_pp_callbacks : public clang::PPCallbacks {
  clang::DiagnosticsEngine *m_diags;
  llvm::StringRef m_cur_file;

  void report_compilation_error(clang::SourceLocation loc);
  void report_missing_module(clang::SourceLocation loc);

public:
  find_deps_pp_callbacks(clang::DiagnosticsEngine *diags, llvm::StringRef file)
      : m_diags{diags}, m_cur_file{file} {}

  void moduleImport(clang::SourceLocation loc, clang::ModuleIdPath path,
                    const clang::Module *imported) override;
};

class find_deps_action : public clang::PreprocessorFrontendAction {
public:
  void ExecuteAction() override;
};
