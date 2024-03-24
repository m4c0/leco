#include "context.hpp"

using namespace llvm;

context &cur_ctx() {
  static context i{};
  return i;
}
