#pragma once
#include "llvm/ADT/StringRef.h"

[[nodiscard]] std::string link(llvm::StringRef main_src);
