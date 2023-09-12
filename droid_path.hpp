#pragma once
#include "llvm/ADT/SmallVector.h"
#include <system_error>

std::error_code find_android_llvm(llvm::SmallVectorImpl<char> &path);
