#include "cl.hpp"
#include "context.hpp"
#include "evoker.hpp"
#include "link.hpp"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

static void recurse(StringSet<> &uniq, StringRef cur) {
  uniq.insert(cur);
  for (auto &p : cur_ctx().pcm_dep_map[cur.str()].modules) {
    recurse(uniq, p);
  }
}
static void recurse_libs(StringSet<> &uniq, StringRef cur) {
  auto libs = cur_ctx().pcm_dep_map[cur.str()].libraries;
  uniq.insert(libs.begin(), libs.end());
  for (auto &p : cur_ctx().pcm_dep_map[cur.str()].modules) {
    recurse_libs(uniq, p);
  }
}
std::string link(StringRef main_src) {
  StringSet<> mods{};
  for (auto &p : cur_ctx().pcm_reqs) {
    recurse(mods, p);
  }
  StringSet<> libs{};
  for (auto &p : cur_ctx().pcm_reqs) {
    recurse_libs(libs, p);
  }

  std::set<StringRef> fws{};

  std::vector<std::string> args{};
  for (auto &p : mods) {
    SmallString<128> pp{};
    in2out(p.first(), pp, "o");
    args.push_back(pp.str().str());

    for (auto &fw : cur_ctx().pcm_dep_map[p.first().str()].frameworks) {
      fws.insert(fw);
    }
  }

  SmallString<128> exe{};
  in2out(main_src, exe, "exe");

  if (cur_ctx().exe_type == exe_t::app) {
    cur_ctx().app_exe_path(exe, sys::path::stem(main_src));
    sys::fs::create_directories(sys::path::parent_path(exe));
  }

  if (is_verbose()) {
    errs() << "linking " << exe << "\n";
  }

  evoker e{};
  for (auto &p : args) {
    e.push_arg(p);
  }
  for (auto &p : libs) {
    e.push_arg(p.first());
  }
  for (auto &fw : fws) {
    e.push_arg("-framework");
    e.push_arg(fw);
  }
  e.push_arg("-o");
  e.push_arg(exe);
  return e.execute() ? std::string{exe} : std::string{};
}
