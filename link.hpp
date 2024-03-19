#pragma once
#include <string>

namespace dag {
class node;
}
[[nodiscard]] std::string link(const dag::node *n, uint64_t mtime);
