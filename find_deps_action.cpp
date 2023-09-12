#include "compile.hpp"
#include "context.hpp"
#include "find_deps_action.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;

void find_deps_pp_callbacks::report_compilation_error(SourceLocation loc) {
  auto lvl = DiagnosticsEngine::Error;
  auto did = m_diags->getCustomDiagID(lvl, "failed to compile dependency");
  m_diags->Report(loc, did);
}
void find_deps_pp_callbacks::report_missing_module(SourceLocation loc) {
  auto lvl = DiagnosticsEngine::Error;
  auto did = m_diags->getCustomDiagID(lvl, "module not found");
  m_diags->Report(loc, did);
}

[[nodiscard]] static bool compile_wd(StringRef who, StringRef d) {
  cur_ctx().add_pcm_dep(d, who);
  return compile(who.str());
}

void find_deps_pp_callbacks::moduleImport(SourceLocation loc, ModuleIdPath path,
                                          const Module *imported) {
  assert(path.size() == 1 && "path isn't atomic");

  auto dir = sys::path::parent_path(m_cur_file);

  auto &[id, _] = path[0];
  auto mod_name = id->getName();
  auto p = mod_name.find(":");
  SmallString<128> dep{};
  if (p != StringRef::npos) {
    auto me = mod_name.substr(0, p);
    auto part = mod_name.substr(p + 1);

    sys::path::append(dep, dir, me + "-" + part + ".cppm");
    if (compile_wd(dep, m_cur_file))
      return;

  } else {
    auto t = mod_name + ".cppm";
    sys::path::append(dep, dir, t);
    if (!sys::fs::is_regular_file(dep)) {
      dep.clear();
      sys::path::append(dep, "..", mod_name, t);
    }
    if (!sys::fs::is_regular_file(dep.c_str()))
      return report_missing_module(loc);

    if (compile_wd(dep, m_cur_file))
      return;
  }

  report_missing_module(loc);
  return;
}
