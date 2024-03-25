#include "link.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
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

  SmallString<128> exe{};
  if (n->dll()) {
    in2out(main_src, exe, cur_ctx().dll_ext);
  } else {
    in2out(main_src, exe, "exe");
  }

  if (n->app()) {
    cur_ctx().app_exe_path(exe, sys::path::stem(main_src));
    sys::fs::create_directories(sys::path::parent_path(exe));
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
