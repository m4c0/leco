#include "diags.hpp"
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace llvm;

DiagnosticsEngine &diags() {
  static IntrusiveRefCntPtr<DiagnosticOptions> diag_opts{
      new DiagnosticOptions()};
  static IntrusiveRefCntPtr<DiagnosticIDs> diag_ids{new DiagnosticIDs()};
  static auto diag_cli = new TextDiagnosticPrinter(errs(), &*diag_opts);

  static DiagnosticsEngine diags{diag_ids, diag_opts, diag_cli};
  return diags;
}
