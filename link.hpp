#pragma once
#include <stdint.h>

namespace dag {
class node;
}
[[nodiscard]] bool link(const dag::node *n, uint64_t mtime);
