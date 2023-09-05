module;
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

export module leco;

void try_main() { llvm::outs() << "ah ah ah leco lecoooo\n"; }

extern "C" int main(int argc, char **argv) {
  llvm::llvm_shutdown_obj sdo{};

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  llvm::CrashRecoveryContext::Enable();

  try {
    try_main();
    return 0;
  } catch (...) {
    llvm::errs() << "unexpected exception\n";
    return 1;
  }
}
