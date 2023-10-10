#include "cl.hpp"
#include "context.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "link.hpp"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

std::string link(StringRef main_src) {
  StringSet<> mods{};
  cur_ctx().list_unique_mods(mods);

  StringSet<> libs{};
  std::set<StringRef> fws{};

  std::vector<std::string> args{};
  for (auto &p : mods) {
    SmallString<128> pp{};
    in2out(p.first(), pp, "o");
    args.push_back(pp.str().str());

    auto &pdm = cur_ctx().pcm_dep_map[p.first().str()];
    for (auto &fw : pdm.frameworks) {
      fws.insert(fw);
    }
    for (auto &l : pdm.libraries) {
      libs.insert(l);
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
  exe.c_str();

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
  for (auto l : cur_ctx().link_flags) {
    e.push_arg(l);
  }
  e.set_out(exe);
  return e.execute() ? std::string{exe} : std::string{};
}
