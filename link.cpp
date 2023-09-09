#include "context.hpp"
#include "evoker.hpp"
#include "link.hpp"

using namespace llvm;

bool link(StringRef in, context *c) {
  return evoker{}.set_inout(in, ".exe").execute();
}
