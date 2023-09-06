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

    return !!evoker{}
                 .push_arg("-c")
                 .set_inout(pcm.output(), ".o")
                 .run<EmitObjAction>();
  } else {
    return !!evoker{}.push_arg("-c").set_inout(file, ".o").run<EmitObjAction>();
  }
}
