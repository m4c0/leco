#include "compile.hpp"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

bool compile_by_ext(llvm::StringRef path) {
  auto ext = llvm::sys::path::extension(path);
  if (ext == ".cppm") {
    return compile(path);
  } else if (ext == ".cpp") {
    return compile(path);
  } else if (ext == ".m") {
    return compile(path);
  } else {
    return true;
  }
}

bool try_main() {
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

    if (!compile_by_ext(it->path())) {
      return false;
    }
  }
  return true;
}

extern "C" int main(int argc, char **argv) {
  llvm::llvm_shutdown_obj sdo{};

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  llvm::CrashRecoveryContext::Enable();

  try {
    return try_main() ? 0 : 1;
  } catch (...) {
    llvm::errs() << "unexpected exception\n";
    return 1;
  }
}
