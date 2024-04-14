#include "link.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "log.hpp"
#include "mkdir.h"
#include "sim.hpp"
#include <set>

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
    auto [it, added] = t.frameworks.insert(fw);
    if (added) {
      t.args.push_back("-framework");
      t.args.push_back(fw);
    }
  }
  for (auto &lib : n->libraries()) {
    auto [it, added] = t.libraries.insert(lib);
    if (added)
      t.args.push_back("-l" + lib);
  }
  for (auto &lib : n->library_dirs()) {
    auto [id, added] = t.library_dirs.insert(lib);
    if (added)
      t.args.push_back("-L" + lib);
  }
}
std::string link(const dag::node *n, uint64_t mtime) {
  things t{};
  dag::visit(n, true, [&](auto *n) { visit(t, n); });

  sim_sbt exe{};
  in2exe(n, &exe);

  if (mtime < mtime_of(exe.buffer))
    return std::string{exe.buffer};

  vlog("linking", exe.buffer);

  evoker e{};
  for (auto &p : t.args) {
    e.push_arg(p.c_str());
  }
  if (n->dll()) {
    e.push_arg("-shared");
  }
  for (auto l : cur_ctx().link_flags) {
    e.push_arg(l.c_str());
  }
  e.set_out(exe.buffer);
  return e.execute() ? std::string{exe.buffer} : std::string{};
}
