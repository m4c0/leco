#include "cl.hpp"
#include "context.hpp"
#include "llvm/Support/CommandLine.h"
#include "llvm/TargetParser/Host.h"

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

enum targets {
  host,
  apple,
  macosx,
  iphoneos,
  iphonesimulator,
  windows,
  android
};
cl::opt<targets> target(
    "target", cl::desc("Targets of build"),
    cl::values(clEnumVal(host, "Same as host"),
               clEnumVal(apple, "All Apple targets"),
               clEnumVal(macosx, "MacOSX"), clEnumVal(iphoneos, "iPhone OS"),
               clEnumVal(iphonesimulator, "iPhone Simulator"),
               clEnumVal(windows, "Windows 64bits"),
               clEnumVal(android, "All Android targets")),
    cl::cat(leco_cat));
bool for_each_target(bool (*fn)()) {
  const auto run = [&](std::string tgt) {
    cur_ctx() = {.target = tgt};
    return fn();
  };
  switch (target) {
  case host:
    return run(sys::getDefaultTargetTriple());

  case apple:
    return run("x86_64-apple-macosx11.6.0") && run("arm64-apple-ios13.0") &&
           run("x86_64-apple-ios13.0-simulator");
  case macosx:
    return run("x86_64-apple-macosx11.6.0");
  case iphoneos:
    return run("arm64-apple-ios13.0");
  case iphonesimulator:
    return run("x86_64-apple-ios13.0-simulator");

  case windows:
    return run("x86_64-pc-windows-msvc");

  case android:
    return run("aarch64-none-linux-android26") &&
           run("armv7-none-linux-androideabi26") &&
           run("i686-none-linux-android26") &&
           run("x86_64-none-linux-android26");
  };

  return false;
}

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
