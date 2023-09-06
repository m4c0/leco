#pragma once

#include "clang/Frontend/FrontendActions.h"

class find_deps_action : public clang::PreprocessorFrontendAction {
public:
  void ExecuteAction() override;
};
