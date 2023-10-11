#include "dag.hpp"
#include "llvm/Support/FileSystem.h"

using namespace llvm;

dag::node::node(StringRef n) {
  sys::fs::real_path(n, m_source);
  sys::fs::make_absolute(m_source);
}
