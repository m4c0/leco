#include "bouncer.hpp"
#include "compile.hpp"
#include "context.hpp"
#include "link.hpp"
#include "llvm/Support/Path.h"

using namespace llvm;

bool compile_and_link(StringRef path) {
  cur_ctx() = {};

  if (!compile(path))
    return false;

  if (cur_ctx().tool) {
    cur_ctx().main_source = path;
    if (!link())
      return false;
  }

  return true;
}

bool bounce(StringRef path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  if (ext == ".cppm") {
    return (stem.find("-") == StringRef::npos) ? compile_and_link(path) : true;
  }

  if (ext != ".cpp")
    return true;

  return compile_and_link(path);
}
