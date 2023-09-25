#include "bouncer.hpp"
#include "cl.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "evoker.hpp"
#include "instance.hpp"
#include "link.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace llvm;

static bool compile_pending() {
  for (auto f : cur_ctx().pending_compilation)
    if (!compile(f))
      return false;
  return true;
}

void copy_resources(StringRef exe) {
  SmallString<256> path{exe};
  cur_ctx().app_res_path(path);
  sys::fs::create_directories(path);

  StringSet<> mods{};
  cur_ctx().list_unique_mods(mods);
  for (auto &p : mods) {
    for (auto &r : cur_ctx().pcm_dep_map[p.first().str()].resources) {
      sys::path::append(path, sys::path::filename(r));
      if (is_verbose()) {
        errs() << "copying resource " << path << "\n";
      }
      sys::fs::copy_file(r, path);
      sys::path::remove_filename(path);
    }
  }
}
void bundle_app(StringRef exe) {
  SmallString<256> path{exe};
  cur_ctx().bundle(path, sys::path::stem(exe));
}

class bouncer : public PreprocessOnlyAction {
public:
  void EndSourceFileAction() override {
    SmallString<128> pwd;
    sys::fs::current_path(pwd); // TODO: check errors
    auto pwd_stem = sys::path::stem(pwd);
    auto file_stem = sys::path::stem(getCurrentFile());
    auto file_ext = sys::path::extension(getCurrentFile());
    auto &pp = getCompilerInstance().getPreprocessor();
    bool root = pp.isInNamedModule() && pp.getNamedModuleName() == pwd_stem &&
                file_ext == ".cppm";

    auto tool = cur_ctx().exe_type == exe_t::tool && cur_ctx().native_target;
    auto app = cur_ctx().exe_type == exe_t::app;
    auto exe = tool || app;

    if (!root && !exe)
      return;

    cur_ctx().pcm_reqs.clear();
    cur_ctx().pending_compilation.clear();
    if (!compile(getCurrentFile()))
      return;

    if (!compile_pending())
      return;

    if (!exe)
      return;

    cur_ctx().add_pcm_req(getCurrentFile());
    auto exe_path = link(getCurrentFile());
    if (exe_path != "" && app) {
      copy_resources(exe_path);
      bundle_app(exe_path);
    }

    cur_ctx().exe_type = exe_t::none;
  }
};

bool bounce(StringRef path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  if (ext != ".cppm" && ext != ".cpp")
    return true;

  auto ci = evoker{}.set_cpp_std().push_arg("-E").push_arg(path).createCI();
  ci->getDiagnostics().setClient(new IgnoringDiagConsumer());

  bouncer b{};
  return ci->ExecuteAction(b);
}
