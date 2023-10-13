#include "compile.hpp"
#include "evoker.hpp"
#include "instance.hpp"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <set>

using namespace clang;
using namespace llvm;

bool compile(StringRef file) {
  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
    auto pcm = evoker{}
                   .set_cpp_std()
                   .push_arg("--precompile")
                   .set_inout(file, ".pcm")
                   .build();

    if (!pcm.run<GenerateModuleInterfaceAction>())
      return false;

    return evoker{}
        .push_arg("-c")
        .set_inout(pcm.output(), ".o")
        .build()
        .run<EmitObjAction>(false);
  } else if (ext == ".cpp") {
    return evoker{}
        .set_cpp_std()
        .push_arg("-c")
        .set_inout(file, ".o")
        .build()
        .run<EmitObjAction>();
  } else if (ext == ".c") {
    return evoker{}
        .push_arg("-c")
        .set_inout(file, ".o")
        .build()
        .run<EmitObjAction>();
  } else if (ext == ".mm" || ext == ".m") {
    return evoker{}
        .push_arg("-c")
        .push_arg("-fmodules")
        .push_arg("-fobjc-arc")
        .set_inout(file, ".o")
        .build()
        .run<EmitObjAction>(false);
  } else {
    errs() << "don't know how to build " << file << "\n";
    return false;
  }
}
