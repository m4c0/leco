#include "cl.hpp"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

cl::OptionCategory leco_cat("Leco Options", "Options exclusive to LECO");

void parse_args(int argc, char **argv) {
  for (auto &[k, v] : cl::getRegisteredOptions()) {
    if (k == "help")
      continue;
    if (v->Categories[0] != &leco_cat)
      v->setHiddenFlag(cl::Hidden);
  }

  cl::ParseCommandLineOptions(argc, argv, "Uga buga");
}
