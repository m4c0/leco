#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

void compile(llvm::StringRef path);

void try_main() {
  std::error_code ec;
  for (llvm::sys::fs::directory_iterator it{".", ec}, e; it != e;
       it.increment(ec)) {
    auto status = it->status();
    if (!status) {
      llvm::errs() << it->path() << ": " << status.getError().message() << "\n";
      continue;
    }

    if (status->type() != llvm::sys::fs::file_type::regular_file) {
      continue;
    }

    auto ext = llvm::sys::path::extension(it->path());
    if (ext == ".cppm") {
      compile(it->path());
    } else if (ext == ".cpp") {
      compile(it->path());
    } else if (ext == ".m") {
      compile(it->path());
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
