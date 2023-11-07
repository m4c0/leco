#include "cl.hpp"
#include "compile.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "pragma.hpp"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <set>

using namespace clang;
using namespace llvm;

class wrapped : public clang::WrapperFrontendAction {
public:
  using WrapperFrontendAction::WrapperFrontendAction;

  bool BeginSourceFileAction(CompilerInstance &ci) override {
    ci.getPreprocessor().AddPragmaHandler(new ns_pragma());
    return WrapperFrontendAction::BeginSourceFileAction(ci);
  }
};

static bool gen_pcm(const evoker &e) {
  auto ci = e.createCI();
  if (!ci)
    return false;

  wrapped w{std::make_unique<GenerateModuleInterfaceAction>()};
  return ci->ExecuteAction(w);
}
static bool emit_obj(const evoker &e) {
  auto ci = e.createCI();
  if (!ci)
    return false;

  wrapped w{std::make_unique<EmitObjAction>()};
  return ci->ExecuteAction(w);
}
static bool emit_obj_simpler(const evoker &e) {
  auto ci = e.createCI();
  if (!ci)
    return false;

  EmitObjAction a{};
  return ci->ExecuteAction(a);
}

bool compile(const dag::node *n) {
  auto file = n->source();
  auto obj = n->target();

  auto path = sys::path::parent_path(obj);
  sys::fs::create_directories(path);

  if (is_verbose()) {
    errs() << "compiling " << obj << "\n";
  }

  auto ext = sys::path::extension(file);
  if (ext == ".cppm") {
    SmallString<256> pcm{obj};
    sys::path::replace_extension(pcm, "pcm");
    pcm.c_str();

    if (!gen_pcm(evoker{}
                     .set_cpp_std()
                     .push_arg("--precompile")
                     .push_arg(file)
                     .set_out(pcm)
                     .pull_deps_from(n)))
      return false;

    return emit_obj_simpler(evoker{}.push_arg("-c").push_arg(pcm).set_out(obj));
  } else if (ext == ".cpp") {
    return emit_obj(evoker{}
                        .set_cpp_std()
                        .push_arg("-c")
                        .push_arg(file)
                        .set_out(obj)
                        .pull_deps_from(n));
  } else if (ext == ".c") {
    return emit_obj(evoker{}.push_arg("-c").push_arg(file).set_out(obj));
  } else if (ext == ".mm" || ext == ".m") {
    return emit_obj_simpler(evoker{}
                                .push_arg("-c")
                                .push_arg("-fmodules")
                                .push_arg("-fobjc-arc")
                                .push_arg(file)
                                .set_out(obj));
  } else {
    errs() << "don't know how to build " << file << "\n";
    return false;
  }
}
