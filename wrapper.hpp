#pragma once
#include "clang/Frontend/FrontendActions.h"

class wrapper_action : public clang::WrapperFrontendAction {
public:
  using WrapperFrontendAction::WrapperFrontendAction;
  bool BeginSourceFileAction(clang::CompilerInstance &ci) override;
};
