#include "cl.hpp"
#include "context.hpp"
#include "target_defs.hpp"
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
cl::opt<bool> x_verbose("extra-verbose",
                        cl::desc("Output as much actions as possible"),
                        cl::cat(leco_cat));
bool is_verbose() { return verbose || x_verbose; }
bool is_extra_verbose() { return x_verbose; }

cl::opt<bool> debug("debug-syms", cl::desc("Enable debug symbols"),
                    cl::cat(leco_cat));
bool enable_debug_syms() { return debug; }

cl::opt<bool> optimise("opt", cl::desc("Enable optimisations"),
                       cl::cat(leco_cat));
bool is_optimised() { return optimise; }

cl::opt<bool> dump_dag("dump-dag", cl::desc("Dump the dependency graph"),
                       cl::cat(leco_cat));
bool is_dumping_dag() { return dump_dag; }

enum targets {
  host,
  apple,
  macosx,
  ios,
  iphoneos,
  iphonesimulator,
  linux,
  windows,
  android
};
cl::opt<targets> target(
    "target", cl::desc("Targets of build"),
    cl::values(clEnumVal(host, "Same as host"),
               clEnumVal(apple, "All Apple targets"),
               clEnumVal(ios, "All iOS targets (iPhone OS + Simulator)"),
               clEnumVal(macosx, "MacOSX"), clEnumVal(iphoneos, "iPhone OS"),
               clEnumVal(iphonesimulator, "iPhone Simulator"),
               clEnumVal(linux, "Linux"),
               clEnumVal(windows, "Windows 64bits"),
               clEnumVal(android, "All Android targets")),
    cl::cat(leco_cat));
bool for_each_target(bool (*fn)()) {
  const auto run = [&](auto &&ctx_fn) {
    cur_ctx() = ctx_fn();
    return fn();
  };

  switch (target) {
#ifdef __APPLE__
  case apple:
    return run(t::macosx) && run(t::iphoneos) && run(t::iphonesimulator);

  case ios:
    return run(t::iphoneos) && run(t::iphonesimulator);

  case host:
  case macosx:
    return run(t::macosx);
  case iphoneos:
    return run(t::iphoneos);
  case iphonesimulator:
    return run(t::iphonesimulator);
#endif

#ifdef _WIN32
  case host:
  case windows:
    return run(t::windows);
#endif

#ifdef __linux__
  case host:
  case linux:
    return run(t::linux);
#endif

  case android:
    return run(t::android_aarch64) && run(t::android_armv7) &&
           run(t::android_i686) && run(t::android_x86_64);

  default:
    return false;
  };
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
