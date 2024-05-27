#include "../mtime/mtime.h"
#include "context.hpp"
#include "dag.hpp"
#include "die.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"

#include <filesystem>

void prep(sim_sb *cmd, const char *tool);

static void in2exe(const dag::node *n, sim_sb *out) {
  std::string ext = n->dll() ? cur_ctx().dll_ext : "exe";
  in2out(n->source(), out, ext.c_str(), cur_ctx().target.c_str());

  if (n->app()) {
    sim_sbt stem{};
    sim_sb_path_copy_sb_stem(&stem, out);

    cur_ctx().app_exe_path(out, stem.buffer);

    sim_sbt path{};
    sim_sb_path_copy_parent(&path, out->buffer);
    mkdirs(path.buffer);
  }
}

static void copy_exe(const char *log, const sim_sb *ef, const char *exe_path) {
  sim_sbt path{};
  sim_sb_copy(&path, exe_path);
  sim_sb_path_append(&path, sim_sb_path_filename(ef));

  if (mtime_of(path.buffer) >= mtime_of(ef->buffer))
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
static void copy_build_deps(const dag::node *n, const char *exe_path) {
  for (const auto &d : n->build_deps()) {
    auto dn = dag::get_node(d.c_str());

    sim_sbt exe{};
    in2exe(dn, &exe);

    copy_exe("copying dependency", &exe, exe_path);
  }
}

static void copy(const char *with, const dag::node *n, const char *to) {
  sim_sbt cmd{};
  prep(&cmd, with);
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, n->dag());
  sim_sb_concat(&cmd, " -o ");
  sim_sb_concat(&cmd, to);
  run(cmd.buffer);
}

void bundle(const dag::node *n) {
  sim_sbt exe{};
  sim_sb_copy(&exe, n->dag());
  sim_sb_path_set_extension(&exe, "exe");

  sim_sbt res_path{};
  sim_sb_copy(&res_path, exe.buffer);
  cur_ctx().app_res_path(&res_path);
  mkdirs(res_path.buffer);

  sim_sbt exe_path{};
  in2exe(n, &exe_path);
  sim_sb_path_parent(&exe_path);

  copy_build_deps(n, exe_path.buffer);
  copy("leco-exs.exe", n, exe_path.buffer);
  copy("leco-rsrc.exe", n, res_path.buffer);

  cur_ctx().bundle(exe.buffer, n->module_name());
}
