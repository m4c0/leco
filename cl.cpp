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
bool should_clean_current() { return clean_level >= cup_cur; }
bool should_clean_all() { return clean_level >= cup_all; }

cl::opt<bool> verbose("verbose", cl::desc("Output important actions"),
                      cl::cat(leco_cat));
bool is_verbose() { return verbose; }

void parse_args(int argc, char **argv) {
  for (auto &[k, v] : cl::getRegisteredOptions()) {
    if (k == "help")
      continue;
    if (v->Categories[0] != &leco_cat)
      v->setHiddenFlag(cl::Hidden);
  }

  cl::ParseCommandLineOptions(
      argc, argv, "This is too cool and it doesn't require description");
}
