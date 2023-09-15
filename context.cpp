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
void context::add_pcm_framework(StringRef path, StringRef fw) {
  pcm_dep_map[to_abs(path)].frameworks.insert(fw.str());
}
void context::add_pcm_resource(StringRef path, StringRef res) {
  pcm_dep_map[to_abs(path)].resources.insert(to_abs(res));
}
void context::add_pcm_library(StringRef path, StringRef lib) {
  pcm_dep_map[to_abs(path)].resources.insert(to_abs(lib));
}

void context::add_pending(StringRef f) {
  pending_compilation.insert(to_abs(f));
}
