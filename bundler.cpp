#include "../mtime/mtime.h"
#include "context.hpp"
#include "dag.hpp"
#include "in2exe.hpp"
#include "log.hpp"
#include "mkdir.h"

#include <filesystem>

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
void copy_build_deps(const dag::node *n) {
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

bool bundle(const dag::node *n, const char *exe_path) {
  sim_sbt res_path{};
  sim_sb_copy(&res_path, exe_path);
  cur_ctx().app_res_path(&res_path);
  mkdirs(res_path.buffer);

  bool success = true;
  dag::visit(n, true, [&](auto *n) {
    success &= compile_shaders(n, res_path.buffer);
    copy_exes(n, exe_path);
    copy_resources(n, res_path.buffer);
  });
  if (!success)
    return false;

  cur_ctx().bundle(exe_path, n->module_name());
  return true;
}
