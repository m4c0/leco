#ifdef _WIN32
#define EXE_EXT ".exe"
#else
#define EXE_EXT ""
#endif

#include "cl.hpp"
#include "clang_dir.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
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

static std::shared_ptr<CompilerInstance> createCI(ArrayRef<const char *> args) {
  auto tgt = cur_ctx().target;

  if (is_extra_verbose()) {
    errs() << "create compiler instance for args (target = [" << tgt << "]):\n";
    for (auto a : args)
      errs() << a << " ";
    errs() << "\n";
  }

  auto clang = std::make_shared<CompilerInstance>();

  auto diags = ::diags();

  Driver drv{clang_exe(), tgt, diags};
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

evoker::evoker() {
  m_args.push_back(clang_exe());
  m_args.push_back("-Wall");
  m_args.push_back("-target");
  m_args.push_back(cur_ctx().target.c_str());

  if (is_optimised()) {
    m_args.push_back("-O3");
    m_args.push_back("-flto");
    m_args.push_back("-fvisibility=hidden");
  }
  if (enable_debug_syms()) {
    m_args.push_back("-g");
  }

  if (cur_ctx().sysroot != "") {
    m_args.push_back("--sysroot");
    m_args.push_back(cur_ctx().sysroot.c_str());
  }

  for (auto f : cur_ctx().cxx_flags) {
    m_args.push_back(f.str());
  }
}

std::shared_ptr<CompilerInstance> evoker::createCI() const {
  std::vector<const char *> args{};
  for (const auto &s : m_args) {
    args.push_back(s.c_str());
  }

  auto ci = ::createCI({args});

  if (m_node == nullptr)
    return ci;

  auto &mod_files = ci->getHeaderSearchOpts().PrebuiltModuleFiles;
  dag::visit(m_node, [&](auto *n) {
    mod_files.insert({n->module_name().str(), n->module_pcm().str()});
  });

  return ci;
}

bool evoker::execute() {
  std::vector<const char *> args{};
  for (const auto &s : m_args) {
    args.push_back(s.c_str());
  }
  if (is_extra_verbose()) {
    errs() << "execute compiler with args (target = [" << cur_ctx().target
           << "]):\n";
    for (auto a : args)
      errs() << a << " ";
    errs() << "\n";
  }

  auto diags = ::diags();
  Driver drv{clang_exe(), cur_ctx().target, diags};
  std::unique_ptr<Compilation> c{drv.BuildCompilation(args)};
  if (!c || c->containsError() || c->getJobs().size() == 0)
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return {};

  SmallVector<std::pair<int, const Command *>, 4> fails;
  int res = drv.ExecuteCompilation(*c, fails);
  for (auto &p : fails) {
    if (res == 0)
      res = p.first;
  }
  return res == 0;
}
