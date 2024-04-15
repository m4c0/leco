#include "link.hpp"

#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "sim.hpp"

#include <set>

bool link(const dag::node *n, const char *exe) {
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
