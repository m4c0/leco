#include "compile.hpp"
#include "evoker.hpp"
#include "find_deps_action.hpp"
#include "instance.hpp"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace llvm;

StringSet<> &already_built() {
  static StringSet<> i{};
  return i;
}

bool try_compile(StringRef file) {
  SmallString<64> out{};
  auto parent = sys::path::parent_path(file);
  sys::path::append(out, parent, "out");

  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
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
    auto bld = evoker{}
                   .push_arg("-c")
                   .set_inout(file, ".o")
                   .build()
                   .add_module_path(out);

    return bld.run<find_deps_action>() && bld.run<EmitObjAction>();
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

bool compile(StringRef file, bool clear_cache) {
  if (clear_cache) {
    already_built().clear();
  }

  if (already_built().contains(file))
    return true;
  if (!try_compile(file))
    return false;

  already_built().insert(file);
  return true;
}
