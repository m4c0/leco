#include "wrapper.hpp"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

bool wrapper_action::BeginSourceFileAction(CompilerInstance &ci) {
  return WrapperFrontendAction::BeginSourceFileAction(ci);
}
