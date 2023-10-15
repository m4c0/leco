#include "bouncer.hpp"
#include "cl.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "instance.hpp"
#include "link.hpp"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace llvm;

static bool compile_shaders(const dag::node *n, StringRef res_path) {
  for (auto &s : n->shaders()) {
    StringRef in = s.first();

    SmallString<256> out{};
    sys::path::append(out, res_path, sys::path::filename(in));
    out.append(".spv");

    auto cmd = ("glslangValidator --quiet -V -o " + out + " " + in).str();
    if (0 != system(cmd.c_str()))
      return false;
  }
  return true;
}
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
  if (!n) {
    errs() << "Unparsed path: [" << path << "]\n";
    return false;
  }

  if (n->compiled())
    return true;

  // Recurse dependencies
  for (auto &d : n->mod_deps()) {
    if (!lets_do_it(d.first()))
      return false;
  }

  // Compile self
  if (!compile(n))
    return false;

  // Compile impls
  for (auto &d : n->mod_impls()) {
    dag::node n{d.first()};
    if (!compile(&n))
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

    bool success = true;
    dag::visit(n, [&](auto *n) {
      success &= compile_shaders(n, res_path);
      copy_exes(n, exe_path);
      copy_resources(n, res_path);
    });
    if (!success)
      return false;

    bundle_app(exe_path);
  }

  return true;
}
