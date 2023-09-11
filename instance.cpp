#include "cl.hpp"
#include "context.hpp"
#include "instance.hpp"
#include "wrapper.hpp"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

auto &cleaned_up_paths() {
  static std::set<std::string> i{};
  return i;
}
auto &current_paths() {
  static std::set<std::string> i{};
  return i;
}
auto &in_flights() {
  static std::vector<std::shared_ptr<CompilerInstance>> i{};
  return i;
}

void clear_module_path_cache() { current_paths().clear(); }

instance::instance(std::shared_ptr<CompilerInstance> ci, StringRef out)
    : m_ci{std::move(ci)}, m_output{out.str()} {

  auto &inf = in_flights();

  if (m_ci) {
    for (auto &p : current_paths()) {
      m_ci->getHeaderSearchOpts().AddPrebuiltModulePath(p);
    }

    if (is_verbose()) {
      errs() << "compiling " << out << "\n";
    }
    inf.push_back(m_ci);
  }

  auto path = sys::path::parent_path(out);
  auto [_, inserted] = current_paths().insert(path.str());

  if (!inserted)
    return;

  auto [__, not_cleaned] = cleaned_up_paths().insert(path.str());
  if (not_cleaned) {
    if ((should_clean_current() && cleaned_up_paths().size() == 1) ||
        should_clean_all()) {
      if (is_verbose())
        errs() << "cleaning up " << path << "\n";
      sys::fs::remove_directories(path);
    }
  }
  sys::fs::create_directories(path);

  for (auto &ci : inf) {
    ci->getHeaderSearchOpts().AddPrebuiltModulePath(path);
  }
}
instance::~instance() {
  if (m_ci)
    std::erase(in_flights(), m_ci);
}

bool instance::run_wo_ctx(std::unique_ptr<FrontendAction> a) {
  return m_ci ? m_ci->ExecuteAction(*a) : false;
}
bool instance::run(std::unique_ptr<FrontendAction> a) {
  if (!m_ci)
    return false;

  wrapper_action fd{std::move(a)};
  cur_ctx().object_files.insert(m_output);
  return m_ci->ExecuteAction(fd);
}

StringRef instance::output() { return m_output; }
