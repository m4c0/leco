#include "evoker.hpp"
#include "find_deps_action.hpp"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace llvm;

bool compile(StringRef file) {
  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
    SmallString<64> out{};
    auto parent = sys::path::parent_path(file);
    sys::path::append(out, parent, "out");

    auto pcm = evoker{}
                   .push_arg("--precompile")
                   .set_inout(file, ".pcm")
                   .build()
                   .add_module_path(out);
    if (!pcm.run<find_deps_action>())
      return false;
    if (!pcm.run<GenerateModuleInterfaceAction>())
      return false;

    return compile(pcm.output());
  } else if (ext == ".cpp") {
    // TODO: detect module impls
    return !!evoker{}
                 .push_arg("-c")
                 .set_inout(file, ".o")
                 .build()
                 .run<EmitObjAction>();
  } else if (ext == ".c" || ext == ".pcm" || ext == ".m") {
    return !!evoker{}
                 .push_arg("-c")
                 .set_inout(file, ".o")
                 .build()
                 .run<EmitObjAction>();
  } else {
    errs() << "don't know how to build " << file << "\n";
    return false;
  }
}
