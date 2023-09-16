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

  const auto rec = [&](auto &rec, const std::string &m) -> void {
    for (auto &r : cur_ctx().pcm_dep_map[m].resources) {
      sys::path::append(path, sys::path::filename(r));
      if (is_verbose()) {
        errs() << "copying resource " << path << "\n";
      }
      sys::fs::copy_file(r, path);
      sys::path::remove_filename(path);
    }
    for (auto &m : cur_ctx().pcm_dep_map[m].modules) {
      rec(rec, m);
    }
  };
  for (auto &p : cur_ctx().pcm_reqs) {
    rec(rec, p);
  }
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
    auto exe = tool || (cur_ctx().exe_type == exe_t::app);

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
    if (exe_path != "")
      copy_resources(exe_path);

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
