#include "context.hpp"

context &cur_ctx() {
  static context i{};
  return i;
}
