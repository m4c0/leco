#include "context.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"

using namespace llvm;

context &cur_ctx() {
  static context i{};
  return i;
}
