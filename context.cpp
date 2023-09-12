#include "context.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"

using namespace llvm;

context &cur_ctx() {
  static context i{};
  return i;
}
static std::string to_abs(StringRef path) {
  SmallString<128> buf{};
  sys::fs::real_path(path, buf);
  sys::fs::make_absolute(buf);
  return buf.c_str();
}
void context::add_pcm_req(StringRef path) { pcm_reqs.insert(to_abs(path)); }
void context::add_pcm_dep(StringRef from, StringRef to) {
  pcm_dep_map[to_abs(from)].modules.insert(to_abs(to));
}
