#include "find_deps_action.hpp"
#include "p_add_impl.hpp"
#include "wrapper.hpp"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

bool wrapper_action::BeginSourceFileAction(CompilerInstance &ci) {
  auto *diags = &ci.getDiagnostics();
  auto &pp = ci.getPreprocessor();

  pp.addPPCallbacks(
      std::make_unique<find_deps_pp_callbacks>(diags, getCurrentFile()));

  pp.AddPragmaHandler("leco", new add_impl_pragma_handler(getCurrentFile()));
  pp.AddPragmaHandler("leco",
                      new add_framework_pragma_handler(getCurrentFile()));

  return WrapperFrontendAction::BeginSourceFileAction(ci);
}
