#include "bouncer.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "llvm/Support/Path.h"

using namespace llvm;

bool bounce(StringRef path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  cur_ctx() = {};

  if (ext == ".cppm") {
    return (stem.find("-") == StringRef::npos) ? compile(path) : true;
  }

  if (ext != ".cpp")
    return true;

  return compile(path); // TODO: link as well
}
