#pragma once
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Chrono.h"

static auto mod_time(llvm::Twine file) {
  llvm::sys::fs::file_status s{};
  if (llvm::sys::fs::status(file, s))
    return llvm::sys::TimePoint<>{};

  return s.getLastModificationTime();
}
