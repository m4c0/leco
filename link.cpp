#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "link.hpp"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct things {
  StringSet<> visited{};
  StringSet<> frameworks{};
  std::vector<std::string> args{};
};
} // namespace

static std::string i2o(StringRef src) {
  SmallString<128> pp{};
  in2out(src, pp, "o");
  return pp.str().str();
}

static void recurse(things &t, const dag::node *n) {
  if (t.visited.contains(n->source()))
    return;

  for (auto &d : n->mod_deps()) {
    auto *dn = dag::get_node(d.first());
    recurse(t, dn);
  }

  t.args.push_back(i2o(n->source()));

  for (auto &i : n->mod_impls()) {
    t.args.push_back(i2o(i.first()));
  }
  for (auto &fw : n->frameworks()) {
    if (t.frameworks.contains(fw.first()))
      continue;
    t.frameworks.insert(fw.first());
    t.args.push_back("-framework");
    t.args.push_back(fw.first().str());
  }

  t.visited.insert(n->source());
}
std::string link(const dag::node *n) {
  auto main_src = n->source();

  things t{};
  recurse(t, n);
  /*
  StringSet<> libs{};
  std::set<StringRef> fws{};

  for (auto &p : mods) {
    auto &pdm = cur_ctx().pcm_dep_map[p.first().str()];
    for (auto &fw : pdm.frameworks) {
      fws.insert(fw);
    }
    for (auto &l : pdm.libraries) {
      libs.insert(l);
    }
  }
  */

  SmallString<128> exe{};
  in2out(main_src, exe, "exe");

  if (n->app()) {
    cur_ctx().app_exe_path(exe, sys::path::stem(main_src));
    sys::fs::create_directories(sys::path::parent_path(exe));
  }

  if (is_verbose()) {
    errs() << "linking " << exe << "\n";
  }
  exe.c_str();

  evoker e{};
  for (auto &p : t.args) {
    e.push_arg(p);
  }
  /*
  for (auto &p : libs) {
    e.push_arg(p.first());
  }
  for (auto &fw : fws) {
    e.push_arg("-framework");
    e.push_arg(fw);
  }
  for (auto l : cur_ctx().link_flags) {
    e.push_arg(l);
  }
  */
  e.set_out(exe);
  return e.execute() ? std::string{exe} : std::string{};
}
