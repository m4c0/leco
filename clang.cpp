#define GOPT_IMPLEMENTATION
#include "../gopt/gopt.h"

#define SIM_IMPLEMENTATION
#include "sim.h"

#include "host_target.hpp"

static void clang_cmd(sim_sb *buf, const char *exe) {
#if __APPLE__
  sim_sb_copy(buf, "/usr/local/opt/llvm@16/bin");
  sim_sb_path_append(buf, exe);
#elif _WIN32
  sim_sb_copy(buf, exe);
  sim_sb_concat(buf, ".exe");
#else
  sim_sb_copy(buf, exe);
#endif
}

int usage() {
  // TODO: print usage
  fprintf(stderr, "invalid usage\n");
  return 1;
}

bool add_target_defs(sim_sb *buf, const char *tgt) {
  if (0 == strcmp(tgt, "x86_64-pc-windows-msvc")) {
    sim_sb_concat(buf, " -DLECO_TARGET_WINDOWS");
  } else if (0 == strcmp(tgt, "x86_64-pc-linux-gnu")) {
    sim_sb_concat(buf, " -DLECO_TARGET_LINUX");
  } else if (0 == strcmp(tgt, "x86_64-apple-macosx11.6.0")) {
    sim_sb_concat(buf, " -DLECO_TARGET_MACOSX");
    sim_sb_concat(buf, " -DLECO_TARGET_APPLE");
    sim_sb_concat(buf, " -D_C99_SOURCE");
  } else if (0 == strcmp(tgt, "arm64-apple-ios16.1")) {
    sim_sb_concat(buf, " -DLECO_TARGET_IPHONEOS");
    sim_sb_concat(buf, " -DLECO_TARGET_IOS");
    sim_sb_concat(buf, " -DLECO_TARGET_APPLE");
    sim_sb_concat(buf, " -D_C99_SOURCE");
  } else if (0 == strcmp(tgt, "x86_64-apple-ios16.1-simulator")) {
    sim_sb_concat(buf, " -DLECO_TARGET_IPHONESIMULATOR");
    sim_sb_concat(buf, " -DLECO_TARGET_IOS");
    sim_sb_concat(buf, " -DLECO_TARGET_APPLE");
    sim_sb_concat(buf, " -D_C99_SOURCE");
  } else if (0 == strcmp(tgt, "aarch64-none-linux-android26")) {
    sim_sb_concat(buf, " -DLECO_TARGET_ANDROID");
  } else if (0 == strcmp(tgt, "armv7-none-linux-androideabi26")) {
    sim_sb_concat(buf, " -DLECO_TARGET_ANDROID");
  } else if (0 == strcmp(tgt, "i686-none-linux-android26")) {
    sim_sb_concat(buf, " -DLECO_TARGET_ANDROID");
  } else if (0 == strcmp(tgt, "x86_64-none-linux-android26")) {
    sim_sb_concat(buf, " -DLECO_TARGET_ANDROID");
  } else {
    return false;
  }
  return true;
}

int main(int argc, char **argv) {
  struct gopt opts;
  GOPT(opts, argc, argv, "gi:Ot:");

  bool debug{};
  bool opt{};
  bool cpp = true;
  const char *target{HOST_TARGET};
  const char *input{};
  const char *ext{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'g':
      debug = true;
      break;
    case 'i': {
      input = val;
      ext = sim_path_extension(input);
      cpp = (0 == strcmp(ext, ".cpp")) || (0 == strcmp(ext, ".cppm")) ||
            (0 == strcmp(ext, ".mm"));
      break;
    }
    case 'O':
      opt = true;
      break;
    case 't':
      target = val;
      break;
    default:
      return usage();
    }
  }

  sim_sb args{};
  sim_sb_new(&args, 10240);
  if (cpp) {
    clang_cmd(&args, "clang++");
    sim_sb_concat(&args, " -std=c++2b");
  } else {
    clang_cmd(&args, "clang");
    sim_sb_concat(&args, " -std=c11");
  }
  sim_sb_concat(&args, " -Wall -Wno-unknown-pragmas");

  if (0 == strcmp(ext, ".m") || 0 == strcmp(ext, ".mm")) {
    sim_sb_concat(&args, " -fmodules -fobjc-arc");
  }

  if (debug) {
    sim_sb_concat(&args, " -g");
  }
  if (opt) {
    sim_sb_concat(&args, " -O3 -flto -fvisibility=hidden");
  }

  sim_sb_concat(&args, " -target ");
  sim_sb_concat(&args, target);
  if (!add_target_defs(&args, target))
    return usage();

  if (input) {
    sim_sb_concat(&args, " ");
    sim_sb_concat(&args, input);
  }

  for (auto i = 0; i < opts.argc; i++) {
    // TODO: escape argv
    sim_sb_concat(&args, " ");
    sim_sb_concat(&args, opts.argv[i]);
  }

  // Somehow, `system` might return 256 and our own return do a mod 256
  return 0 == system(args.buffer) ? 0 : 1;
}
