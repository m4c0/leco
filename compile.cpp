#include "cl.hpp"
#include "compile.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "pragma.hpp"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <set>

using namespace clang;
using namespace llvm;

bool compile(const dag::node *n) {
  auto file = n->source();
  auto obj = n->target();

  auto path = sys::path::parent_path(obj);
  sys::fs::create_directories(path);

  if (is_verbose()) {
    errs() << "compiling " << obj << "\n";
  }

  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
    SmallString<256> pcm{obj};
    sys::path::replace_extension(pcm, "pcm");
    pcm.c_str();

    if (!evoker{}
             .set_cpp_std()
             .add_predefs()
             .push_arg("--precompile")
             .push_arg(file)
             .set_out(pcm)
             .pull_deps_from(n)
             .push_arg("-fplugin=../leco/null_pragma.dll")
             .execute())
      return false;

    return evoker{}
        .push_arg("-c")
        .push_arg(pcm)
        .set_out(obj)
        .pull_deps_from(n)
        .execute();
  } else if (ext == ".cpp") {
    return evoker{}
        .set_cpp_std()
        .add_predefs()
        .push_arg("-c")
        .push_arg(file)
        .set_out(obj)
        .pull_deps_from(n)
        .push_arg("-fplugin=../leco/null_pragma.dll")
        .execute();
  } else if (ext == ".c") {
    return evoker{}
        .push_arg("-c")
        .push_arg(file)
        .set_out(obj)
        .add_predefs()
        .push_arg("-fplugin=../leco/null_pragma.dll")
        .execute();
  } else if (ext == ".mm" || ext == ".m") {
    return evoker{}
        .push_arg("-c")
        .push_arg("-fmodules")
        .push_arg("-fobjc-arc")
        .push_arg(file)
        .set_out(obj)
        .execute();
  } else {
    errs() << "don't know how to build " << file << "\n";
    return false;
  }
}
