#pragma once
#include "llvm/ADT/StringRef.h"

void clear_compile_cache();
bool compile(llvm::StringRef file);
