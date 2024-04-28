#include "bouncer.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2exe.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "phase2.hpp"
#include "sim.hpp"

#include <filesystem>

static bool link(const dag::node *n, const char *exe) {
  struct things {
    std::set<std::string> frameworks{};
    std::set<std::string> libraries{};
    std::set<std::string> library_dirs{};
  } t{};

  evoker e{};
  e.set_out(exe);
#ifdef _WIN32 // otherwise, face LNK1107 errors from MSVC
  e.push_arg("-fuse-ld=lld");
#endif
  if (n->dll()) {
    e.push_arg("-shared");
  }
  for (auto l : cur_ctx().link_flags) {
    e.push_arg(l.c_str());
  }

  dag::visit(n, true, [&](auto *n) {
    e.push_arg(n->target());

    for (auto &fw : n->frameworks()) {
      auto [it, added] = t.frameworks.insert(fw);
      if (added) {
        e.push_arg("-framework").push_arg(fw.c_str());
      }
    }
    for (auto &lib : n->libraries()) {
      auto [it, added] = t.libraries.insert(lib);
      if (added)
        e.push_arg("-l").push_arg(lib.c_str());
    }
    for (auto &lib : n->library_dirs()) {
      auto [id, added] = t.library_dirs.insert(lib);
      if (added)
        e.push_arg("-L").push_arg(lib.c_str());
    }
  });

  return e.execute();
}

bool bounce(const char *path) {
  auto ext = sim_path_extension(path);
  if (ext == nullptr)
    return true;

  if (strcmp(ext, ".cppm") != 0 && strcmp(ext, ".cpp") != 0)
    return true;

  auto n = dag::process(path);
  if (!n)
    return false;
  if (!n->root())
    return true;

  if (n->tool() && !cur_ctx().native_target)
    return true;

  for (const auto &d : n->build_deps()) {
    if (!bounce(d.c_str()))
      return false;

    copy_build_deps(n);
  }

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

  sim_sbt exe_path{};
  in2exe(n, &exe_path);

  if (mtime > mtime_of(exe_path.buffer)) {
    vlog("linking", exe_path.buffer);
    link(n, exe_path.buffer);
    return true;
  }

  if (n->app()) {
    bundle(n, exe_path.buffer);
  }

  return true;
}
