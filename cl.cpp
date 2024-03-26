#include "cl.hpp"
#include "../gopt/gopt.h"
#include "context.hpp"
#include "target_defs.hpp"

#include <stdio.h>
#include <string.h>

enum clean_levels { cup_none = 0, cup_cur, cup_all };
static clean_levels clean_level{};
bool should_clean_current() { return clean_level >= cup_cur; }
bool should_clean_all() { return clean_level >= cup_all; }

static int verbose{};
bool is_verbose() { return verbose > 0; }
bool is_extra_verbose() { return verbose > 1; }

static bool debug{};
bool enable_debug_syms() { return debug; }

static bool optimise{};
bool is_optimised() { return optimise; }

static bool dump_dag{};
bool is_dumping_dag() { return dump_dag; }

enum targets {
  host = 0,
  apple,
  macosx,
  ios,
  iphoneos,
  iphonesimulator,
  linux,
  windows,
  android
};
static targets target;
bool parse_target(const char *n) {
  constexpr const char *vals[]{"host",  "apple",    "macosx",
                               "ios",   "iphoneos", "iphonesimulator",
                               "linux", "windows",  "android"};
  int i = 0;
  for (auto x : vals) {
    if (strcmp(x, n) == 0) {
      target = static_cast<targets>(i);
      return true;
    }
    i++;
  }
  return false;
}
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

bool usage() {
  fprintf(stderr, R"(
  Usage: ../leco/leco.exe [-c|-C] [-D] [-g] [-O] [-t <target>] [-v]

  Where:
    -c -- clean current module before build (mutually exclusive with -C)

    -C -- clean all modules before build (mutually exclusive with -c)

    -D -- dump DAG (useful for troubleshooting LECO itself or module dependencies)

    -g -- enable debug symbols

    -O -- enable optimisations

    -t <target> -- one of:
      iphoneos, iphonesimulator: for its referring platform (requires Apple SDKs)
      ios: for both iPhoneOS and iPhoneSimulator
      android: for all four Android architectures (requires Android SDK)
      apple, linux, macosx, windows: for their respective platforms (requires their SDKs)
      host: for the same platform as the host (default)

    -v -- enable verboseness (repeat for extra verbosiness)

)");
  return false;
}

bool parse_args(int argc, char **argv) {
  struct gopt opts {};
  GOPT(opts, argc, argv, "cCDgOt:v");

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'c':
      clean_level = cup_cur;
      break;
    case 'C':
      clean_level = cup_all;
      break;
    case 'D':
      dump_dag = true;
      break;
    case 'g':
      debug = true;
      break;
    case 'O':
      optimise = true;
      break;
    case 't':
      if (!parse_target(val))
        return false;
      break;
    case 'v':
      verbose++;
      break;
    default:
      return usage();
    }
  }

  if (opts.argc != 0)
    return usage();

  return true;
}
