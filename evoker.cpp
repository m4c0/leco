#ifdef _WIN32
#define EXE_EXT ".exe"
#else
#define EXE_EXT ""
#endif

#include "clang_dir.hpp"
#include "context.hpp"
#include "evoker.hpp"
#include "instance.hpp"
#include "clang/CodeGen/ObjectFilePCHContainerOperations.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"

using namespace clang;
using namespace clang::driver;
using namespace llvm;

const char *clang_exe() {
  static const auto exe = [] {
    SmallString<1024> buf{};
    sys::path::append(buf, clang_dir(), "bin", "clang++" EXE_EXT);
    return buf;
  }();
  return exe.data();
}

static DiagnosticsEngine diags() {
  IntrusiveRefCntPtr<DiagnosticOptions> diag_opts{new DiagnosticOptions()};
  IntrusiveRefCntPtr<DiagnosticIDs> diag_ids{new DiagnosticIDs()};
  auto diag_cli = new TextDiagnosticPrinter(errs(), &*diag_opts);

  return DiagnosticsEngine{diag_ids, diag_opts, diag_cli};
}

std::shared_ptr<CompilerInstance> createCI(ArrayRef<const char *> args) {
  auto clang = std::make_shared<CompilerInstance>();

  auto diags = ::diags();

  Driver drv{clang_exe(), cur_ctx().target, diags};
  std::unique_ptr<Compilation> c{drv.BuildCompilation(args)};
  if (!c || c->containsError() || c->getJobs().size() == 0)
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return {};

  auto cc1args = c->getJobs().begin()->getArguments();
  if (!CompilerInvocation::CreateFromArgs(clang->getInvocation(), cc1args,
                                          diags))
    return {};

  clang->createDiagnostics();

  for (auto def : cur_ctx().predefs)
    clang->getPreprocessorOpts().addMacroDef(def);

  return clang;
}

void in2out(llvm::StringRef in, llvm::SmallVectorImpl<char> &out,
            llvm::StringRef ext) {
  out.clear();

  auto path = sys::path::parent_path(in);
  auto gpath = sys::path::parent_path(path);
  if (sys::path::stem(gpath) != "out") {
    auto name = sys::path::stem(in);
    auto triple = cur_ctx().target;
    sys::path::append(out, path, "out", triple, name);
  } else {
    sys::path::append(out, in);
  }
  // TODO: check errors
  sys::fs::make_absolute(out);
  sys::path::replace_extension(out, ext);
}

evoker::evoker() {
  m_args.push_back(clang_exe());
  m_args.push_back("-std=c++20");
  m_args.push_back("-target");
  m_args.push_back(cur_ctx().target.c_str());

  if (cur_ctx().sysroot != "") {
    m_args.push_back("--sysroot");
    m_args.push_back(cur_ctx().sysroot.c_str());
  }
}
evoker &evoker::set_inout(StringRef in, StringRef ext) {
  in2out(in, m_obj, ext);

  m_args.push_back(in.data());
  m_args.push_back("-o");
  m_args.push_back(m_obj.c_str());
  return *this;
}
instance evoker::build() { return instance{createCI(), m_obj.str()}; }

std::shared_ptr<CompilerInstance> evoker::createCI() {
  return ::createCI(m_args);
}

bool evoker::execute() {
  auto diags = ::diags();
  Driver drv{clang_exe(), cur_ctx().target, diags};
  std::unique_ptr<Compilation> c{drv.BuildCompilation(m_args)};
  if (!c || c->containsError() || c->getJobs().size() == 0)
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return {};

  SmallVector<std::pair<int, const Command *>, 4> fails;
  auto res = drv.ExecuteCompilation(*c, fails);
  for (auto &p : fails) {
    if (!res)
      res = p.first;
  }
  return res;
}
