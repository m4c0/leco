#include "compile.hpp"
#include "context.hpp"
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

auto &already_built() {
  static std::set<std::string> i{};
  return i;
}

bool try_compile(StringRef file, context *c) {
  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
    auto pcm =
        evoker{}.push_arg("--precompile").set_inout(file, ".pcm").build();

    if (!pcm.run<GenerateModuleInterfaceAction>(c))
      return false;

    return evoker{}
        .push_arg("-c")
        .set_inout(pcm.output(), ".o")
        .build()
        .run<EmitObjAction>(nullptr);
  } else if (ext == ".cpp" || ext == ".c" || ext == ".pcm" || ext == ".m") {
    return evoker{}
        .push_arg("-c")
        .set_inout(file, ".o")
        .build()
        .run<EmitObjAction>(c);
  } else {
    errs() << "don't know how to build " << file << "\n";
    return false;
  }
}

std::string make_abs(StringRef file) {
  SmallString<1024> buf;
  // TODO: check errors
  sys::fs::real_path(file, buf);
  return buf.str().str();
}
bool compile(StringRef rel_file) {
  auto file = make_abs(rel_file);

  if (already_built().contains(file))
    return true;

  context c{};
  if (!try_compile(file, &c))
    return false;

  if (c.tool) {
    outs() << "tool: " << file << "\n";
  }

  already_built().insert(file);
  return true;
}

void clear_compile_cache() { already_built().clear(); }
