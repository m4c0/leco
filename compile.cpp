#include "evoker.hpp"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace llvm;

bool compile(StringRef file) {
  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
    auto pcm = evoker{}.push_arg("--precompile").set_inout(file, ".pcm");
    if (!pcm.run<GenerateModuleInterfaceAction>())
      return false;

    return compile(pcm.output());
  } else if (ext == ".c" || ext == ".pcm" || ext == ".m") {
    return !!evoker{}.push_arg("-c").set_inout(file, ".o").run<EmitObjAction>();
  }
}
