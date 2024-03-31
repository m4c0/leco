#include "compile.hpp"

#include "dag.hpp"
#include "evoker.hpp"
#include "log.hpp"
#include "sim.hpp"
#include "llvm/Support/FileSystem.h"
#include <set>

using namespace clang;
using namespace llvm;

bool compile(const dag::node *n) {
  auto file = n->source();
  auto obj = n->target();

  sim_sbt path{256};
  sim_sb_path_copy_parent(&path, obj);
  sys::fs::create_directories(path.buffer);

  vlog("compiling", obj);

  // TODO: remove extra copy when "obj" becomes a sim_sb
  sim_sbt f2{256};
  sim_sb_copy(&f2, file);
  auto ext = sim_sb_path_extension(&f2);
  if (strcmp(ext, ".cppm") == 0) {
    auto pcm = n->module_pcm();

    if (!evoker{"--precompile", file, pcm}
             .set_cpp_std()
             .add_predefs()
             .pull_deps_from(n)
             .suppress_pragmas()
             .execute())
      return false;

    return evoker{"-c", pcm, obj}.pull_deps_from(n).execute();
  } else if (strcmp(ext, ".cpp") == 0) {
    return evoker{"-c", file, obj}
        .set_cpp_std()
        .add_predefs()
        .pull_deps_from(n)
        .suppress_pragmas()
        .execute();
  } else if (strcmp(ext, ".c") == 0) {
    return evoker{"-c", file, obj}.add_predefs().suppress_pragmas().execute();
  } else if (strcmp(ext, ".m") == 0 || strcmp(ext, ".mm") == 0) {
    return evoker{"-c", file, obj}
        .push_arg("-fmodules")
        .push_arg("-fobjc-arc")
        .suppress_pragmas()
        .execute();
  } else {
    fprintf(stderr, "don't know how to build %s\n", file);
    return false;
  }
}
