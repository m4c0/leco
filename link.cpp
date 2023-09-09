#include "context.hpp"
#include "evoker.hpp"
#include "link.hpp"

using namespace llvm;

bool link(context *c) {
  std::vector<std::string> args{};
  for (auto &p : *(c->module_paths)) {
    args.push_back("-fprebuilt-module-path=" + p);
  }

  auto e = evoker{}.set_inout(c->main_obj, ".exe");
  for (auto &p : args) {
    e.push_arg(p);
  }
  return e.execute();
}
