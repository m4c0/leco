#include "instance.hpp"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

StringSet<> &module_paths() {
  static StringSet<> i{};
  return i;
}

instance::instance(std::shared_ptr<CompilerInstance> ci, StringRef out)
    : m_ci{std::move(ci)}, m_output{out.str()} {}
instance::~instance() = default;

instance &instance::add_module_path(StringRef path) {
  module_paths().insert(path);
  return *this;
}

bool instance::run(FrontendAction *a) {
  if (!m_ci)
    return false;

  for (auto &p : module_paths())
    m_ci->getHeaderSearchOpts().AddPrebuiltModulePath(p.first());

  return m_ci->ExecuteAction(*a);
}

StringRef instance::output() { return m_output; }
