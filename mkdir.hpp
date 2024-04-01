#pragma once
#include "llvm/Support/FileSystem.h"

static void mkdirs(const char *path) {
  llvm::sys::fs::create_directories(path);
}
