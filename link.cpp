#include "link.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "sim.hpp"
#include "llvm/Support/FileSystem.h"
#include <set>

using namespace llvm;

namespace {
struct things {
  std::set<std::string> frameworks{};
  std::set<std::string> libraries{};
  std::set<std::string> library_dirs{};
  std::vector<std::string> args{};
};
} // namespace

static void visit(things &t, const dag::node *n) {
  t.args.push_back(n->target());

  for (auto &fw : n->frameworks()) {
    auto [it, added] = t.frameworks.insert(fw.first().str());
    if (added) {
      t.args.push_back("-framework");
      t.args.push_back(fw.first().str());
    }
  }
  for (auto &lib : n->libraries()) {
    auto [it, added] = t.libraries.insert(lib.first().str());
    if (added)
      t.args.push_back("-l" + lib.first().str());
  }
  for (auto &lib : n->library_dirs()) {
    auto [id, added] = t.library_dirs.insert(lib.first().str());
    if (added)
      t.args.push_back("-L" + lib.first().str());
  }
}
std::string link(const dag::node *n, uint64_t mtime) {
  things t{};
  dag::visit(n, true, [&](auto *n) { visit(t, n); });

  std::string ext = n->dll() ? cur_ctx().dll_ext : "exe";
  sim_sbt exe{256};
  in2out(n->source_sb(), &exe, ext.c_str());

  if (n->app()) {
    sim_sbt stem{256};
    sim_sb_path_copy_sb_stem(&stem, &exe);

    cur_ctx().app_exe_path(&exe, stem.buffer);

    sim_sbt path{256};
    sim_sb_path_copy_parent(&path, exe.buffer);
    sys::fs::create_directories(path.buffer);
  }

  if (mtime < mtime_of(exe.buffer))
    return std::string{exe.buffer};

  vlog("linking", exe.buffer);

  evoker e{};
  for (auto &p : t.args) {
    e.push_arg(p);
  }
  if (n->dll()) {
    e.push_arg("-shared");
  }
  for (auto l : cur_ctx().link_flags) {
    e.push_arg(l);
  }
  e.set_out(exe.buffer);
  return e.execute() ? std::string{exe.buffer} : std::string{};
}
