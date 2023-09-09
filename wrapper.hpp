#pragma once
#include "clang/Frontend/FrontendActions.h"

class wrapper_action : public clang::WrapperFrontendAction {

public:
  wrapper_action(std::unique_ptr<FrontendAction> a)
      : WrapperFrontendAction{std::move(a)} {}

  bool BeginSourceFileAction(clang::CompilerInstance &ci) override;
};
