#include "cl.hpp"
#include "../gopt/gopt.h"
#include "context.hpp"
#include "die.hpp"
#include "target_defs.hpp"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

static const char *target{"host"};

static int clean_level{};
bool should_clean_current() { return clean_level > 0; }
bool should_clean_all() { return clean_level > 1; }

static int verbose{1};
bool is_verbose() { return verbose > 0; }
bool is_extra_verbose() { return verbose > 1; }

static bool debug{};
bool enable_debug_syms() { return debug; }

static bool optimise{};
bool is_optimised() { return optimise; }

bool for_each_target(bool (*fn)()) {
  const auto run = [&](auto &&ctx_fn) {
    cur_ctx() = ctx_fn();
    return fn();
  };

#ifdef __APPLE__
  if (0 == strcmp(target, "apple"))
    return run(t::macosx) && run(t::iphoneos) && run(t::iphonesimulator);

  if (0 == strcmp(target, "ios"))
    return run(t::iphoneos) && run(t::iphonesimulator);

  if (0 == strcmp(target, "host") || 0 == strcmp(target, "macosx"))
    return run(t::macosx);
  if (0 == strcmp(target, "iphoneos"))
    return run(t::iphoneos);
  if (0 == strcmp(target, "iphonesimulator"))
    return run(t::iphonesimulator);
#endif

#ifdef _WIN32
  if (0 == strcmp(target, "host") || 0 == strcmp(target, "windows"))
    return run(t::windows);
#endif

#ifdef __linux__
  if (0 == strcmp(target, "host") || 0 == strcmp(target, "linux"))
    return run(t::linux);
#endif

  if (0 == strcmp(target, "ios"))
    return run(t::android_aarch64) && run(t::android_armv7) &&
           run(t::android_i686) && run(t::android_x86_64);

  die("unknown target: %s", target);
  return false;
}

bool parse_args(int argc, char **argv) {
  struct gopt opts {};
  GOPT(opts, argc, argv, "C:cgqOt:v");

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'c':
      clean_level++;
      break;
    case 'C':
      if (0 != chdir(val)) {
        fprintf(stderr, "Directory not found: [%s]\n", val);
        return false;
      }
      break;
    case 'g':
      debug = true;
      break;
    case 'O':
      optimise = true;
      break;
    case 'q':
      verbose = 0;
      break;
    case 't':
      target = val;
      break;
    case 'v':
      verbose = 2;
      break;
    default:
      return false;
    }
  }

  if (opts.argc != 0)
    return false;

  return true;
}
