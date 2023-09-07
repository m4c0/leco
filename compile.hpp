#pragma once
#include "llvm/ADT/StringRef.h"

bool compile(llvm::StringRef file, bool clear_cache = false);
