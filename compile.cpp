#include "compile.hpp"
#include "evoker.hpp"
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
  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
    auto pcm =
        evoker{}.push_arg("--precompile").set_inout(file, ".pcm").build();

    if (!pcm.run<GenerateModuleInterfaceAction>())
      return false;

    return compile(pcm.output());
  } else if (ext == ".cpp" || ext == ".c" || ext == ".pcm" || ext == ".m") {
    auto bld = evoker{}.push_arg("-c").set_inout(file, ".o").build();

    return !!bld.run<EmitObjAction>();
  } else {
    errs() << "don't know how to build " << file << "\n";
    return false;
  }
}

bool compile(StringRef file) {
  if (already_built().contains(file))
    return true;
  if (!try_compile(file))
    return false;

  already_built().insert(file);
  return true;
}

void clear_compile_cache() { already_built().clear(); }
