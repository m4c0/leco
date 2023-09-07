#include "instance.hpp"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

instance::instance(std::shared_ptr<CompilerInstance> ci, StringRef out)
    : m_ci{std::move(ci)}, m_output{out.str()} {}
instance::~instance() = default;

instance &instance::add_module_path(StringRef path) {
  m_ci->getHeaderSearchOpts().AddPrebuiltModulePath(path);
  return *this;
}

bool instance::run(FrontendAction *a) {
  return m_ci && m_ci->ExecuteAction(*a);
}

StringRef instance::output() { return m_output; }
