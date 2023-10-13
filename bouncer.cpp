#include "bouncer.hpp"
#include "cl.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "instance.hpp"
#include "link.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace llvm;

/*
void copy_resources(StringRef exe) {
  SmallString<256> path{exe};
  cur_ctx().app_res_path(path);
  sys::fs::create_directories(path);

  StringSet<> mods{};
  cur_ctx().list_unique_mods(mods);
  for (auto &p : mods) {
    for (auto &r : cur_ctx().pcm_dep_map[p.first().str()].resources) {
      sys::path::append(path, sys::path::filename(r));
      if (is_verbose()) {
        errs() << "copying resource " << path << "\n";
      }
      sys::fs::copy_file(r, path);
      sys::path::remove_filename(path);
    }
  }

  path = exe;
  for (auto &p : mods) {
    for (auto &e : cur_ctx().pcm_dep_map[p.first().str()].executables) {
      sys::path::remove_filename(path);
      sys::path::append(path, sys::path::filename(e));
      if (is_verbose()) {
        errs() << "copying library " << path << "\n";
      }
      sys::fs::copy_file(e, path);
    }
  }
}
void bundle_app(StringRef exe) {
  SmallString<256> path{exe};
  cur_ctx().bundle(path, sys::path::stem(exe));
}
*/

bool lets_do_it(StringRef path) {
  auto *n = dag::get_node(path);
  if (!n)
    return false;

  if (n->compiled())
    return true;

  // Recurse dependencies
  for (auto &d : n->mod_deps()) {
    if (!lets_do_it(d.first()))
      return false;
  }

  // Compile self
  if (!compile(path))
    return false;

  // Compile impls
  for (auto &d : n->mod_impls()) {
    if (!compile(d.first()))
      return false;
  }

  n->set_compiled();
  return true;
}

bool bounce(StringRef path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  if (ext != ".cppm" && ext != ".cpp")
    return true;

  auto n = dag::process(path);
  if (!n)
    return false;
  if (!n->root())
    return true;

  if (n->tool() && !cur_ctx().native_target)
    return true;

  if (!lets_do_it(n->source()))
    return false;

  if (!n->app() && !n->tool())
    return true;

  auto exe_path = link(n);
  if (exe_path != "" && n->app()) {
    // copy_resources(exe_path);
    // bundle_app(exe_path);
  }

  return true;
}
