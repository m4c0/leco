#include "cl.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "link.hpp"
#include "mtime.hpp"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct things {
  StringSet<> frameworks{};
  StringSet<> libraries{};
  StringSet<> library_dirs{};
  std::vector<std::string> args{};
};
} // namespace

static std::string i2o(StringRef src) {
  SmallString<128> pp{};
  in2out(src, pp, "o");
  return pp.str().str();
}

static void visit(things &t, const dag::node *n) {
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
  for (auto &lib : n->libraries()) {
    if (t.libraries.contains(lib.first()))
      continue;
    t.libraries.insert(lib.first());
    t.args.push_back("-l" + lib.first().str());
  }
  for (auto &lib : n->library_dirs()) {
    if (t.library_dirs.contains(lib.first()))
      continue;
    t.library_dirs.insert(lib.first());
    t.args.push_back("-L" + lib.first().str());
  }
}
std::string link(const dag::node *n, sys::TimePoint<> mtime) {
  if (n->dll()) {
    errs() << "TODO: dll linking\n";
    return "";
  }

  auto main_src = n->source();

  things t{};
  dag::visit(n, [&](auto *n) { visit(t, n); });

  SmallString<128> exe{};
  in2out(main_src, exe, "exe");

  if (n->app()) {
    cur_ctx().app_exe_path(exe, sys::path::stem(main_src));
    sys::fs::create_directories(sys::path::parent_path(exe));
  }

  if (mtime < mod_time(exe))
    return std::string{exe};

  if (is_verbose()) {
    errs() << "linking " << exe << "\n";
  }
  exe.c_str();

  evoker e{};
  for (auto &p : t.args) {
    e.push_arg(p);
  }
  for (auto l : cur_ctx().link_flags) {
    e.push_arg(l);
  }
  e.set_out(exe);
  return e.execute() ? std::string{exe} : std::string{};
}
