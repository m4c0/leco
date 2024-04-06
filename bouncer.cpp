#include "bouncer.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "cleaner.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "link.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"

#include <filesystem>
#include <string.h>

static bool compile_shaders(const dag::node *n, const char *res_path) {
  for (auto &s : n->shaders()) {
    // TODO: remove when set is sim
    sim_sbt in{};
    sim_sb_copy(&in, s.c_str());

    sim_sbt out{};
    sim_sb_path_copy_append(&out, res_path, sim_sb_path_filename(&in));
    sim_sb_concat(&out, ".spv");

    if (mtime_of(out.buffer) > mtime_of(in.buffer))
      continue;

    vlog("compiling shader", out.buffer);

    sim_sbt cmd{1024};
    sim_sb_printf(&cmd, "glslangValidator -V -o %s %s", out.buffer, in.buffer);
    if (0 != system(cmd.buffer))
      return false;
  }
  return true;
}
static void copy_exes(const dag::node *n, const char *exe_path) {
  const auto &rpath = cur_ctx().rpath;

  for (auto &e : n->executables()) {
    // TODO: remove when set is sim
    sim_sbt ef{};
    sim_sb_copy(&ef, e.c_str());

    sim_sbt path{};
    sim_sb_path_copy_parent(&path, exe_path);
    if (rpath != "") {
      sim_sb_path_append(&path, rpath.c_str());
      mkdirs(path.buffer);
    }
    sim_sb_path_append(&path, sim_sb_path_filename(&ef));

    if (mtime_of(path.buffer) > mtime_of(ef.buffer))
      continue;

    vlog("copying library", path.buffer);
    std::filesystem::copy_file(ef.buffer, path.buffer);
  }
}
static void copy_resources(const dag::node *n, const char *res_path) {
  for (auto &r : n->resources()) {
    // TODO: remove when set is sim
    sim_sbt rf{};
    sim_sb_copy(&rf, r.c_str());

    sim_sbt path{};
    sim_sb_path_copy_append(&path, res_path, sim_sb_path_filename(&rf));

    if (mtime_of(path.buffer) > mtime_of(rf.buffer))
      continue;

    vlog("copying resource", path.buffer);
    std::filesystem::copy_file(rf.buffer, path.buffer);
  }
}

bool bounce(const char *path) {
  sim_sbt pp{};
  sim_sb_copy(&pp, path);

  auto ext = sim_sb_path_extension(&pp);
  if (ext == nullptr)
    return true;

  if (strcmp(ext, ".cppm") != 0 && strcmp(ext, ".cpp") != 0)
    return true;

  auto n = dag::process(path);
  if (!n)
    return false;
  if (!n->root())
    return true;

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
    sim_sbt res_path{};
    sim_sb_copy(&res_path, exe_path.c_str());
    cur_ctx().app_res_path(&res_path);
    mkdirs(res_path.buffer);

    bool success = true;
    dag::visit(n, true, [&](auto *n) {
      success &= compile_shaders(n, res_path.buffer);
      copy_exes(n, exe_path.c_str());
      copy_resources(n, res_path.buffer);
    });
    if (!success)
      return false;

    sim_sbt stem{};
    sim_sb_path_copy_sb_stem(&stem, &pp);
    cur_ctx().bundle(exe_path.c_str(), stem.buffer);
  }

  return true;
}
