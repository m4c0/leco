#include "cl.hpp"
#include "find_deps_action.hpp"
#include "instance.hpp"
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

instance::instance(std::shared_ptr<CompilerInstance> ci, StringRef out)
    : m_ci{std::move(ci)}, m_output{out.str()} {

  auto &inf = in_flights();

  if (m_ci) {
    for (auto &p : current_paths()) {
      m_ci->getHeaderSearchOpts().AddPrebuiltModulePath(p);
    }

    inf.push_back(m_ci);
  }

  auto path = sys::path::parent_path(out);
  auto [_, inserted] = current_paths().insert(path.str());

  if (!inserted)
    return;

  auto [__, not_cleaned] = cleaned_up_paths().insert(path.str());
  if (not_cleaned) {
    if (should_clean_current() && cleaned_up_paths().size() == 1) {
      errs() << "cleaning up " << path << "\n";
      sys::fs::remove_directories(path);
    } else if (should_clean_all()) {
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

bool instance::run(std::unique_ptr<FrontendAction> a, context *ctx) {
  if (!m_ci)
    return false;

  if (ctx != nullptr) {
    find_deps_action fd{std::move(a)};
    return m_ci->ExecuteAction(fd);
  } else {
    return m_ci->ExecuteAction(*a);
  }
}

StringRef instance::output() { return m_output; }
