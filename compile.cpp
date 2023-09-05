#include "llvm/Support/raw_ostream.h"

void compile(llvm::StringRef path) {
  llvm::errs() << "compiling " << path << "\n";
}
