#include "bouncer.hpp"
#include "evoker.hpp"
#include "instance.hpp"
#include "clang/Frontend/FrontendActions.h"

using namespace clang;
using namespace llvm;

class bouncer : public PreprocessorFrontendAction {
public:
  void ExecuteAction() override {}

  [[nodiscard]] bool is_valid() { return false; }
};

bool is_valid_root_compilation(StringRef path) {
  auto stem = sys::path::stem(path);
  auto ext = sys::path::extension(path);

  if (ext == ".cppm")
    return stem.find("-") == StringRef::npos;

  if (ext != ".cpp")
    return false;

  auto b = evoker{}.push_arg("-c").set_inout(path, ".o").build().run<bouncer>();
  return b && b->is_valid();
}
