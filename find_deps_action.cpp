#include "compile.hpp"
#include "context.hpp"
#include "diags.hpp"
#include "find_deps_action.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;
using namespace llvm;

void find_deps_pp_callbacks::moduleImport(SourceLocation loc, ModuleIdPath path,
                                          const Module *imported) {
  assert(path.size() == 1 && "path isn't atomic");

  const auto compile_wd = [&](StringRef who, StringRef d) -> bool {
    cur_ctx().add_pcm_dep(d, who);
    // diag_remark(*m_diags, loc, "compiling dependency");
    return compile(who.str());
  };

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
    if (!sys::fs::is_regular_file(dep)) {
      SmallString<64> camel{mod_name};
      size_t p = 0;
      while ((p = camel.find('_', p)) != StringRef::npos) {
        camel[p] = '-';
      }

      dep.clear();
      sys::path::append(dep, "..", camel, t);
    }
    if (!sys::fs::is_regular_file(dep.c_str())) {
      diag_error(*m_diags, loc, "file not found");
      return;
    }

    if (compile_wd(dep, m_cur_file))
      return;
  }

  diag_error(*m_diags, loc, "failure compiling dependency");
}
