#pragma once
#include "llvm/Support/Chrono.h"
#include <string>

namespace dag {
class node;
}
[[nodiscard]] std::string link(const dag::node *n,
                               llvm::sys::TimePoint<> mtime);
