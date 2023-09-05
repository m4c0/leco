module;
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

export module leco;

void try_main() {
  using dirent = llvm::sys::fs::directory_iterator;
  std::error_code ec;
  dirent it{".", ec};
  while (it != dirent{}) {
    llvm::errs() << it->path() << "\n";
    it.increment(ec);
  }
}

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
