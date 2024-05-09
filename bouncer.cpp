#include "bouncer.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "die.hpp"
#include "evoker.hpp"
#include "in2exe.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "phase2.hpp"
#include "sim.hpp"

#include <filesystem>

extern const char *leco_argv0;
static void compile(const dag::node *n) {
  sim_sbt path{};
  sim_sb_path_copy_parent(&path, n->target());
  mkdirs(path.buffer);

  vlog("compiling", n->source());

  sim_sbt cmd{};
  sim_sb_path_copy_parent(&cmd, leco_argv0);
  sim_sb_path_append(&cmd, "leco-clang.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->source());
  if (enable_debug_syms()) {
    sim_sb_concat(&cmd, " -g");
  }
  if (is_optimised()) {
    sim_sb_concat(&cmd, " -O");
  }
  run(cmd.buffer);
}

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

void bounce(const char *path) {
  auto ext = sim_path_extension(path);
  if (ext == nullptr)
    return;

  if (strcmp(ext, ".cppm") != 0 && strcmp(ext, ".cpp") != 0)
    return;

  auto n = dag::process(path);
  if (!n->root())
    return;

  if (n->tool() && !cur_ctx().native_target)
    return;

  for (const auto &d : n->build_deps()) {
    bounce(d.c_str());
    copy_build_deps(n);
  }

  auto mtime = dag::visit_dirty(n, &compile);

  if (!n->app() && !n->tool() && !n->dll())
    return;

  sim_sbt exe_path{};
  in2exe(n, &exe_path);

  if (mtime > mtime_of(exe_path.buffer)) {
    vlog("linking", exe_path.buffer);
    link(n, exe_path.buffer);
  }

  if (n->app()) {
    bundle(n, exe_path.buffer);
  }
}
