#pragma once
#include <stdint.h>

namespace dag {
class node;
}
[[nodiscard]] bool link(const dag::node *n, const char *exe);
