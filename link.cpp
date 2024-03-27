#include "link.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "sim.h"
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
  t.args.push_back(n->target().str());

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
  auto main_src = n->source();

  things t{};
  dag::visit(n, true, [&](auto *n) { visit(t, n); });

  std::string ext = n->dll() ? cur_ctx().dll_ext : "exe";
  SmallString<128> exe{};
  in2out(main_src, exe, ext);

  if (n->app()) {
    sim_sbt exe2{256};
    sim_sb_copy(&exe2, exe.c_str());

    sim_sbt stem{256};
    sim_sb_path_copy_sb_stem(&stem, &exe2);

    cur_ctx().app_exe_path(exe, stem.buffer);

    sim_sb_path_copy_parent(&exe2, exe.c_str());
    sys::fs::create_directories(exe2.buffer);
  }

  if (mtime < mtime_of(exe.c_str()))
    return std::string{exe};

  vlog("linking", exe.data(), exe.size());
  exe.c_str();

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
  e.set_out(exe);
  return e.execute() ? std::string{exe} : std::string{};
}
