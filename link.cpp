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

  std::vector<std::string> args{};
  for (auto &p : uniq) {
    SmallString<128> pp{};
    in2out(p.first(), pp, "o");
    args.push_back(pp.str().str());
  }

  SmallString<128> exe{};
  in2out(main_src, exe, "exe");

  evoker e{};
  for (auto &p : args) {
    e.push_arg(p);
  }
  e.push_arg("-o");
  e.push_arg(exe);
  return e.execute();
}
