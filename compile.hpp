#pragma once
#include "llvm/ADT/StringRef.h"

[[nodiscard]] bool compile(llvm::StringRef file);
