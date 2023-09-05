#include "clang_dir.hpp"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
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
    sys::path::append(buf, clang_dir(), "bin", "clang++.exe");
    return buf;
  }();
  return exe.data();
}

bool compile(StringRef path) {
  errs() << "compiling " << path << "\n";

  SmallVector<const char *> args{"-v"};

  auto cinst = std::make_unique<CompilerInstance>();
  CompilerInvocation::CreateFromArgs(cinst->getInvocation(), args,
                                     diag_engine(), clang_exe());
  cinst->createDiagnostics();

  EmitObjAction a{};
  return cinst->ExecuteAction(a);
}
