#pragma once
#include "clang/Frontend/FrontendActions.h"

class context;

class wrapper_action : public clang::WrapperFrontendAction {
  context *m_ctx;

public:
  wrapper_action(context *c, std::unique_ptr<FrontendAction> a)
      : WrapperFrontendAction{std::move(a)}, m_ctx{c} {}

  bool BeginSourceFileAction(clang::CompilerInstance &ci) override;
};
