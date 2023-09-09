#include "cl.hpp"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

cl::OptionCategory leco_cat("Leco Options");

enum clean_levels { cup_none, cup_cur, cup_all };
cl::opt<clean_levels> clean_level(
    "clean", cl::desc("Cleaning level before compilation:"),
    cl::values(clEnumValN(cup_none, "none", "No cleanup, do incremental build"),
               clEnumValN(cup_cur, "cur", "Cleanup current directory"),
               clEnumValN(cup_all, "all", "Cleanup any traversed directory")),
    cl::cat(leco_cat));

void parse_args(int argc, char **argv) {
  for (auto &[k, v] : cl::getRegisteredOptions()) {
    if (k == "help")
      continue;
    if (v->Categories[0] != &leco_cat)
      v->setHiddenFlag(cl::Hidden);
  }

  cl::ParseCommandLineOptions(argc, argv, "Uga buga");
}
