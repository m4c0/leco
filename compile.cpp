#include "compile.hpp"

#include "dag.hpp"
#include "evoker.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"

#include <string.h>

extern const char *leco_argv0;
bool compile(const dag::node *n) {
  auto file = n->source();
  auto obj = n->target();

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, obj);
  mkdirs(path.buffer);

  vlog("compiling", obj);

  // TODO: remove extra copy when "obj" becomes a sim_sb
  auto ext = sim_path_extension(file);
  if (strcmp(ext, ".cppm") == 0) {
    auto pcm = n->module_pcm();

    if (!evoker{"--precompile", file, pcm}
             .set_cpp()
             .pull_deps_from(n)
             .execute())
      return false;

    return evoker{"-c", pcm, obj}.pull_deps_from(n).execute();
  } else if (strcmp(ext, ".cpp") == 0) {
    return evoker{"-c", file, obj}.set_cpp().pull_deps_from(n).execute();
  } else if (strcmp(ext, ".c") == 0 || strcmp(ext, ".m") == 0 ||
             strcmp(ext, ".mm") == 0) {
    sim_sbt clang{};
    sim_sb_path_copy_parent(&clang, leco_argv0);
    sim_sb_path_append(&clang, "leco-clang.exe");
    sim_sb_concat(&clang, " -i ");
    sim_sb_concat(&clang, n->source());
    sim_sb_concat(&clang, " -- -o ");
    sim_sb_concat(&clang, n->target());
    return 0 == system(clang.buffer);
  } else {
    fprintf(stderr, "don't know how to build %s\n", file);
    return false;
  }
}
