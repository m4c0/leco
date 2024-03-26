#include "compile.hpp"

#include "dag.hpp"
#include "evoker.hpp"
#include "log.hpp"
#include "sim.h"
#include "llvm/Support/FileSystem.h"
#include <set>

using namespace clang;
using namespace llvm;

bool compile(const dag::node *n) {
  auto file = n->source();
  auto obj = n->target();

  sim_sbt path{256};
  sim_sb_path_copy_parent(&path, obj.str().c_str());
  sys::fs::create_directories(path.buffer);

  vlog("compiling", obj.data(), obj.size());

  // TODO: remove extra copy when "obj" becomes a sim_sb
  sim_sbt f2{256};
  sim_sb_copy(&f2, file.str().c_str());
  auto ext = sim_sb_path_extension(&f2);
  if (strcmp(ext, ".cppm") == 0) {
    auto pcm = n->module_pcm();

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
        .set_out(n->target())
        .pull_deps_from(n)
        .execute();
  } else if (strcmp(ext, ".cpp") == 0) {
    return evoker{}
        .set_cpp_std()
        .add_predefs()
        .push_arg("-c")
        .push_arg(file)
        .set_out(obj)
        .pull_deps_from(n)
        .push_arg("-fplugin=../leco/null_pragma.dll")
        .execute();
  } else if (strcmp(ext, ".c") == 0) {
    return evoker{}
        .push_arg("-c")
        .push_arg(file)
        .set_out(obj)
        .add_predefs()
        .push_arg("-fplugin=../leco/null_pragma.dll")
        .execute();
  } else if (strcmp(ext, ".m") == 0 || strcmp(ext, ".mm") == 0) {
    return evoker{}
        .push_arg("-c")
        .push_arg("-fmodules")
        .push_arg("-fobjc-arc")
        .push_arg(file)
        .set_out(obj)
        .push_arg("-fplugin=../leco/null_pragma.dll")
        .execute();
  } else {
    fprintf(stderr, "don't know how to build %s\n", file.str().c_str());
    return false;
  }
}
