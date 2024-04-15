#include "link.hpp"

#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "log.hpp"
#include "sim.hpp"

#include <set>

bool link(const dag::node *n, const char *exe) {
  struct things {
    std::set<std::string> frameworks{};
    std::set<std::string> libraries{};
    std::set<std::string> library_dirs{};
    std::vector<std::string> args{};
  } t{};

  dag::visit(n, true, [&](auto *n) {
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
  });

  vlog("linking", exe);

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
  e.set_out(exe);
  return e.execute();
}
