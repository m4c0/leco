#include "context.hpp"
#include "evoker.hpp"
#include "link.hpp"

using namespace llvm;

bool link(StringRef main_src) {
  std::vector<std::string> args{};
  for (auto &p : cur_ctx().object_files) {
    SmallString<128> pp{};
    in2out(p, pp, "o");
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
