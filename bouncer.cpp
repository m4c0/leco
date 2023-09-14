#include "bouncer.hpp"
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
    auto app = cur_ctx().exe_type == exe_t::app && cur_ctx().native_target;

    if (!root && !tool && !app)
      return;

    cur_ctx().pcm_reqs.clear();
    cur_ctx().pending_compilation.clear();
    if (!compile(getCurrentFile()))
      return;

    if (!compile_pending())
      return;

    if (!tool && !app)
      return;

    cur_ctx().add_pcm_req(getCurrentFile());
    link(getCurrentFile());
    cur_ctx().exe_type = exe_t::none;
  }
};

bool bounce(StringRef path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  if (ext != ".cppm" && ext != ".cpp")
    return true;

  auto ci = evoker{}.set_cpp_std().push_arg("-E").push_arg(path).createCI();
  ci->getPreprocessorOpts().SingleFileParseMode = true;
  ci->getDiagnostics().setClient(new IgnoringDiagConsumer());

  bouncer b{};
  return ci->ExecuteAction(b);
}
