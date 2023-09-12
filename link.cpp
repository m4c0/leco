#include "context.hpp"
#include "evoker.hpp"
#include "link.hpp"
#include "llvm/ADT/StringSet.h"

using namespace llvm;

static void recurse(StringSet<> &uniq, StringRef cur) {
  uniq.insert(cur);
  for (auto &p : cur_ctx().pcm_dep_map[cur.str()].modules) {
    recurse(uniq, p);
  }
}
bool link(StringRef main_src) {
  StringSet<> uniq{};
  for (auto &p : cur_ctx().pcm_reqs) {
    recurse(uniq, p);
  }

  std::set<StringRef> fws{};

  std::vector<std::string> args{};
  for (auto &p : uniq) {
    SmallString<128> pp{};
    in2out(p.first(), pp, "o");
    args.push_back(pp.str().str());

    for (auto &fw : cur_ctx().pcm_dep_map[p.first().str()].frameworks) {
      fws.insert(fw);
    }
  }

  SmallString<128> exe{};
  in2out(main_src, exe, "exe");

  evoker e{};
  for (auto &p : args) {
    e.push_arg(p);
  }
  for (auto &fw : fws) {
    e.push_arg("-framework");
    e.push_arg(fw);
  }
  e.push_arg("-o");
  e.push_arg(exe);
  return e.execute();
}
