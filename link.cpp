#include "context.hpp"
#include "evoker.hpp"
#include "link.hpp"

using namespace llvm;

bool link() {
  std::vector<std::string> args{};
  for (auto &p : cur_ctx().object_files) {
    args.push_back(p);
  }

  auto e = evoker{}.set_inout(cur_ctx().main_source, ".exe");
  for (auto &p : args) {
    e.push_arg(p);
  }
  return e.execute();
}
