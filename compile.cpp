#include "compile.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
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
  SmallString<256> obj{};
  in2out(file, obj, "o");

  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
    SmallString<256> pcm{};
    in2out(file, pcm, "pcm");

    if (!evoker{}
             .set_cpp_std()
             .push_arg("--precompile")
             .push_arg(file)
             .set_out(pcm)
             .build()
             .run<GenerateModuleInterfaceAction>())
      return false;

    return evoker{}
        .push_arg("-c")
        .push_arg(pcm)
        .set_out(obj)
        .build()
        .run<EmitObjAction>(false);
  } else if (ext == ".cpp") {
    return evoker{}
        .set_cpp_std()
        .push_arg("-c")
        .push_arg(file)
        .set_out(obj)
        .build()
        .run<EmitObjAction>();
  } else if (ext == ".c") {
    return evoker{}
        .push_arg("-c")
        .push_arg(file)
        .set_out(obj)
        .build()
        .run<EmitObjAction>();
  } else if (ext == ".mm" || ext == ".m") {
    return evoker{}
        .push_arg("-c")
        .push_arg("-fmodules")
        .push_arg("-fobjc-arc")
        .push_arg(file)
        .set_out(obj)
        .build()
        .run<EmitObjAction>(false);
  } else {
    errs() << "don't know how to build " << file << "\n";
    return false;
  }
}
