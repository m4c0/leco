module;
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <filesystem>

export module leco;

namespace fs = std::filesystem;

void try_main() {
  const fs::path pwd = fs::current_path();
  for (auto &e : fs::directory_iterator(pwd)) {
    auto ext = e.path().extension();
    if (ext == ".cppm") {
      llvm::outs() << "cppm: " << e.path().string() << "\n";
    } else if (ext == ".cpp") {
      llvm::outs() << "cpp: " << e.path().string() << "\n";
    } else if (ext == ".m") {
      llvm::outs() << "objc: " << e.path().string() << "\n";
    }
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
