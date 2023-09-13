#include "find_deps_action.hpp"
#include "wrapper.hpp"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

bool wrapper_action::BeginSourceFileAction(CompilerInstance &ci) {
  auto *diags = &ci.getDiagnostics();
  auto &pp = ci.getPreprocessor();

  pp.addPPCallbacks(
      std::make_unique<find_deps_pp_callbacks>(diags, getCurrentFile()));

  return WrapperFrontendAction::BeginSourceFileAction(ci);
}
