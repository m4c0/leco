#include "bouncer.hpp"
#include "cl.hpp"
#include "compile.hpp"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

bool try_main() {
  clear_compile_cache();

  std::error_code ec;
  for (sys::fs::directory_iterator it{".", ec}, e; it != e; it.increment(ec)) {
    auto status = it->status();
    if (!status) {
      errs() << it->path() << ": " << status.getError().message() << "\n";
      continue;
    }

    if (status->type() != sys::fs::file_type::regular_file) {
      continue;
    }

    if (!bounce(it->path())) {
      return false;
    }
  }
  return true;
}

extern "C" int main(int argc, char **argv) {
  llvm_shutdown_obj sdo{};

  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();
  CrashRecoveryContext::Enable();

  parse_args(argc, argv);

  try {
    return try_main() ? 0 : 1;
  } catch (...) {
    errs() << "unexpected exception\n";
    return 1;
  }
}
