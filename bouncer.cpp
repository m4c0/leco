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

static void copy_exes(const dag::node *n, StringRef exe_path) {
  SmallString<256> path{exe_path};

  for (auto &e : n->executables()) {
    sys::path::remove_filename(path);
    sys::path::append(path, sys::path::filename(e.first()));
    if (is_verbose()) {
      errs() << "copying library " << path << "\n";
    }
    sys::fs::copy_file(e.first(), path);
  }
}
static void copy_resources(const dag::node *n, StringRef res_path) {
  SmallString<256> path{res_path};

  for (auto &r : n->resources()) {
    sys::path::append(path, sys::path::filename(r.first()));
    if (is_verbose()) {
      errs() << "copying resource " << path << "\n";
    }
    sys::fs::copy_file(r.first(), path);
    sys::path::remove_filename(path);
  }
}
static void bundle_app(StringRef exe) {
  SmallString<256> path{exe};
  cur_ctx().bundle(path, sys::path::stem(exe));
}

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
    SmallString<256> res_path{exe_path};
    cur_ctx().app_res_path(res_path);
    sys::fs::create_directories(res_path);

    dag::visit(n, [&](auto *n) {
      copy_exes(n, exe_path);
      copy_resources(n, res_path);
    });

    bundle_app(exe_path);
  }

  return true;
}
