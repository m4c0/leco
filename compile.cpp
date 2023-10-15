#include "cl.hpp"
#include "compile.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <set>

using namespace clang;
using namespace llvm;

static auto mod_time(Twine file) {
  sys::fs::file_status s{};
  if (sys::fs::status(file, s))
    return sys::TimePoint<>{};

  return s.getLastModificationTime();
}

bool compile(const dag::node *n) {
  auto file = n->source();

  SmallString<256> obj{};
  in2out(file, obj, "o");

  auto fm = mod_time(file);
  auto fo = mod_time(obj);
  // if (fo > fm) return true;

  auto path = sys::path::parent_path(obj);
  sys::fs::create_directories(path);

  if (is_verbose()) {
    errs() << "compiling " << obj << "\n";
  }

  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
    SmallString<256> pcm{};
    in2out(file, pcm, "pcm");
    pcm.c_str();

    if (!evoker{}
             .set_cpp_std()
             .push_arg("--precompile")
             .push_arg(file)
             .set_out(pcm)
             .run<GenerateModuleInterfaceAction>())
      return false;

    return evoker{}
        .push_arg("-c")
        .push_arg(pcm)
        .set_out(obj)
        .run<EmitObjAction>();
  } else if (ext == ".cpp") {
    return evoker{}
        .set_cpp_std()
        .push_arg("-c")
        .push_arg(file)
        .set_out(obj)
        .run<EmitObjAction>();
  } else if (ext == ".c") {
    return evoker{}
        .push_arg("-c")
        .push_arg(file)
        .set_out(obj)
        .run<EmitObjAction>();
  } else if (ext == ".mm" || ext == ".m") {
    return evoker{}
        .push_arg("-c")
        .push_arg("-fmodules")
        .push_arg("-fobjc-arc")
        .push_arg(file)
        .set_out(obj)
        .run<EmitObjAction>();
  } else {
    errs() << "don't know how to build " << file << "\n";
    return false;
  }
}
