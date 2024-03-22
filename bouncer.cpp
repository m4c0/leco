#include "bouncer.hpp"

#include "cl.hpp"
#include "cleaner.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "link.hpp"
#include "log.hpp"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#define MTIME_IMPLEMENTATION
#include "../mtime/mtime.h"

using namespace llvm;

static bool compile_shaders(const dag::node *n, StringRef res_path) {
  for (auto &s : n->shaders()) {
    StringRef in = s.first();

    SmallString<256> out{};
    sys::path::append(out, res_path, sys::path::filename(in));
    out.append(".spv");

    if (mtime_of(out.c_str()) > mtime_of(in.str().c_str()))
      continue;

    vlog("compiling shader", out.data(), out.size());

    auto cmd = ("glslangValidator --quiet -V -o " + out + " " + in).str();
    if (0 != system(cmd.c_str()))
      return false;
  }
  return true;
}
static void copy_exes(const dag::node *n, StringRef exe_path) {
  const auto &rpath = cur_ctx().rpath;

  for (auto &e : n->executables()) {
    SmallString<256> path{exe_path};
    sys::path::remove_filename(path);
    if (rpath != "") {
      sys::path::append(path, rpath);
      sys::fs::create_directories(path);
    }
    sys::path::append(path, sys::path::filename(e.first()));
    if (mtime_of(path.c_str()) > mtime_of(e.first().str().c_str()))
      continue;

    vlog("copying library", path.data(), path.size());
    sys::fs::copy_file(e.first(), path);
  }
}
static void copy_resources(const dag::node *n, StringRef res_path) {
  for (auto &r : n->resources()) {
    SmallString<256> path{res_path};
    sys::path::append(path, sys::path::filename(r.first()));
    if (mtime_of(path.c_str()) > mtime_of(r.first().str().c_str()))
      continue;
    vlog("copying resource", path.data(), path.size());
    sys::fs::copy_file(r.first(), path);
  }
}
static void bundle_app(StringRef exe) {
  SmallString<256> path{exe};
  cur_ctx().bundle(path, sys::path::stem(exe));
}

bool bounce(const char *path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  if (ext != ".cppm" && ext != ".cpp")
    return true;

  auto n = dag::process(path);
  if (!n)
    return false;
  if (!n->root())
    return true;

  if (is_dumping_dag()) {
    outs() << "-=-=-=-=- " << path;
    if (n->root())
      outs() << " root";
    if (n->app())
      outs() << " app";
    if (n->dll())
      outs() << " dll";
    if (n->tool())
      outs() << " tool";
    outs() << "\n";
    dag::visit(n, [&](auto *n) {
      outs() << "  " << n->module_name() << " ==> " << n->module_pcm() << "\n";
    });
    for (auto &d : n->mod_impls()) {
      auto impl = dag::get_node(d.first());
      outs() << "  -- impl: " << impl->source() << "\n";
      dag::visit(impl, [&](auto *n) {
        outs() << "     " << n->module_name() << " ==> " << n->module_pcm()
               << "\n";
      });
    }
    return true;
  }

  clean(n);

  if (n->tool() && !cur_ctx().native_target)
    return true;

  bool failed = false;
  auto mtime = dag::visit_dirty(n, [&](auto *n) {
    if (failed)
      return;
    failed |= !compile(n);
  });
  if (failed)
    return false;

  if (!n->app() && !n->tool() && !n->dll())
    return true;

  auto exe_path = link(n, mtime);
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
