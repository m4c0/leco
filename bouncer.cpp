#include "bouncer.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"

#include <filesystem>

static void in2exe(const dag::node *n, sim_sb *out) {
  std::string ext = n->dll() ? cur_ctx().dll_ext : "exe";
  in2out(n->source_sb(), out, ext.c_str());

  if (n->app()) {
    sim_sbt stem{};
    sim_sb_path_copy_sb_stem(&stem, out);

    cur_ctx().app_exe_path(out, stem.buffer);

    sim_sbt path{};
    sim_sb_path_copy_parent(&path, out->buffer);
    mkdirs(path.buffer);
  }
}

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
static void copy_exe(const char *log, const sim_sb *ef, const char *exe_path) {
  const auto &rpath = cur_ctx().rpath;

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, exe_path);
  if (rpath != "") {
    sim_sb_path_append(&path, rpath.c_str());
    mkdirs(path.buffer);
  }
  sim_sb_path_append(&path, sim_sb_path_filename(ef));

  if (mtime_of(path.buffer) > mtime_of(ef->buffer))
    return;

  vlog(log, path.buffer);

  if (0 != remove(path.buffer)) {
    // Rename original file. This is a "Windows-approved" way of modifying an
    // open executable.
    sim_sbt bkp{};
    sim_sb_copy(&bkp, path.buffer);
    sim_sb_concat(&bkp, ".bkp");
    remove(bkp.buffer);
    rename(path.buffer, bkp.buffer);
  }
  std::filesystem::copy_file(ef->buffer, path.buffer);
}
static void copy_exes(const dag::node *n, const char *exe_path) {
  for (auto &e : n->executables()) {
    // TODO: remove when set is sim
    sim_sbt ef{};
    sim_sb_copy(&ef, e.c_str());

    copy_exe("copying library", &ef, exe_path);
  }
}
static void copy_build_deps(const dag::node *n) {
  sim_sbt exe_path{};
  in2exe(n, &exe_path);

  for (const auto &d : n->build_deps()) {
    auto dn = dag::get_node(d.c_str());

    sim_sbt exe{};
    in2exe(dn, &exe);

    copy_exe("copying dependency", &exe, exe_path.buffer);
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

static bool link(const dag::node *n, const char *exe) {
  struct things {
    std::set<std::string> frameworks{};
    std::set<std::string> libraries{};
    std::set<std::string> library_dirs{};
  } t{};

  evoker e{};
  e.set_out(exe);
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

  if (mtime < mtime_of(exe_path.buffer))
    return true;

  vlog("linking", exe_path.buffer);
  if (link(n, exe_path.buffer) && n->app()) {
    sim_sbt res_path{};
    sim_sb_copy(&res_path, exe_path.buffer);
    cur_ctx().app_res_path(&res_path);
    mkdirs(res_path.buffer);

    bool success = true;
    dag::visit(n, true, [&](auto *n) {
      success &= compile_shaders(n, res_path.buffer);
      copy_exes(n, exe_path.buffer);
      copy_resources(n, res_path.buffer);
    });
    if (!success)
      return false;

    sim_sbt stem{};
    sim_sb_path_copy_sb_stem(&stem, &pp);
    cur_ctx().bundle(exe_path.buffer, stem.buffer);
  }

  return true;
}
