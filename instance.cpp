#include "cl.hpp"
#include "instance.hpp"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

instance::instance(std::shared_ptr<CompilerInstance> ci, StringRef out)
    : m_ci{std::move(ci)}, m_output{out.str()} {

  if (m_ci) {
    // m_ci->getHeaderSearchOpts().AddPrebuiltModulePath(p);

    if (out != "" && is_verbose()) {
      errs() << "compiling " << out << "\n";
    }
  }

  auto path = sys::path::parent_path(out);
  sys::fs::create_directories(path);
}
instance::~instance() {}

bool instance::run(std::unique_ptr<FrontendAction> a) {
  return m_ci ? m_ci->ExecuteAction(*a) : false;
}

StringRef instance::output() { return m_output; }
