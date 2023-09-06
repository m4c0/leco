#ifdef _WIN32
#define EXE_EXT ".exe"
#else
#define EXE_EXT ""
#endif

#include "clang_dir.hpp"
#include "evoker.hpp"
#include "clang/CodeGen/ObjectFilePCHContainerOperations.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"

using namespace clang;
using namespace clang::driver;
using namespace llvm;

DiagnosticsEngine &diag_engine() {
  static IntrusiveRefCntPtr<DiagnosticOptions> diag_opts{
      new DiagnosticOptions()};
  static IntrusiveRefCntPtr<DiagnosticIDs> diag_ids{new DiagnosticIDs()};
  static auto diag_cli = new TextDiagnosticPrinter(llvm::errs(), &*diag_opts);

  static DiagnosticsEngine diags{diag_ids, diag_opts, diag_cli};
  return diags;
}

const char *clang_exe() {
  static const auto exe = [] {
    SmallString<1024> buf{};
    sys::path::append(buf, clang_dir(), "bin", "clang++" EXE_EXT);
    return buf;
  }();
  return exe.data();
}

std::unique_ptr<CompilerInstance> createCI(ArrayRef<const char *> args) {
  auto clang = std::make_unique<CompilerInstance>();

  auto pch = clang->getPCHContainerOperations();
  pch->registerWriter(std::make_unique<ObjectFilePCHContainerWriter>());
  pch->registerReader(std::make_unique<ObjectFilePCHContainerReader>());

  auto def_triple = sys::getDefaultTargetTriple();
  Driver drv{clang_exe(), def_triple, diag_engine()};
  std::unique_ptr<Compilation> c{drv.BuildCompilation(args)};
  if (!c || c->containsError())
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return {};

  auto cc1args = c->getJobs().getJobs()[0]->getArguments();
  if (!CompilerInvocation::CreateFromArgs(clang->getInvocation(), cc1args,
                                          diag_engine()))
    return {};

  clang->createDiagnostics();
  return clang;
}

evoker::evoker() {
  m_args.push_back(clang_exe());
  m_args.push_back("-std=c++20");
}
evoker &evoker::set_inout(StringRef in, StringRef ext) {
  auto path = sys::path::parent_path(in);
  if (sys::path::stem(path) != "out") {
    auto name = sys::path::stem(in);
    sys::path::append(m_obj, path, "out", name);
  } else {
    m_obj.append(in);
  }
  sys::path::replace_extension(m_obj, ext);

  m_args.push_back(in.data());
  m_args.push_back("-o");
  m_args.push_back(m_obj.c_str());
  return *this;
}
[[nodiscard]] bool evoker::run(clang::FrontendAction *a) {
  errs() << "compiling " << m_obj << "\n";

  auto clang = createCI(m_args);
  return clang && clang->ExecuteAction(*a);
}
