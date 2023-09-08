#include "find_deps_action.hpp"
#include "instance.hpp"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

StringSet<> &module_paths() {
  static StringSet<> i{};
  return i;
}

instance::instance(std::shared_ptr<CompilerInstance> ci, StringRef out)
    : m_ci{std::move(ci)}, m_output{out.str()} {
  module_paths().insert(sys::path::parent_path(out));
}
instance::~instance() = default;

bool instance::run(FrontendAction *a) {
  if (!m_ci)
    return false;

  for (auto &p : module_paths())
    m_ci->getHeaderSearchOpts().AddPrebuiltModulePath(p.first());

  find_deps_action fd{};
  if (!m_ci->ExecuteAction(fd))
    return false;

  for (auto &p : module_paths())
    m_ci->getHeaderSearchOpts().AddPrebuiltModulePath(p.first());

  return m_ci->ExecuteAction(*a);
}

StringRef instance::output() { return m_output; }
