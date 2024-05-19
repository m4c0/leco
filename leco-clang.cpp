#pragma leco tool
#define GOPT_IMPLEMENTATION
#define POPEN_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "../popen/popen.h"
#include "die.hpp"
#include "host_target.hpp"
#include "in2out.hpp"

static const char *argv0;

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

static int usage() {
  // TODO: print usage
  fprintf(stderr, R"(
LECO's heavily-opiniated CLANG runner

Usage: %s [-i <input> [-o <output]] [-t <target>] [-g] [-O] [-v] [-- <clang-flags>]

This tool uses the clang version available via PATH, except on MacOS where it 
requires llvm to be installed via Homebrew.

Where:
      -i <input>     input file. When used, certain flags will be automatically
                     inferred, like compilation type ("-c" v "--precompile"),
                     output filename, etc

      -o <output>    output file. Only works as an override for the output file
                     when the flag "-i" is also specified

      -t <target>    target triple (check this tool's source for list of
                     supported targets)

      -g             enable debug flags

      -O             enable optimisation flags

      -v             print command-line before running it

      <clang-flags>  pass flags as-is to clang
)",
          argv0);
  return 1;
}

static void add_target_defs(sim_sb *buf, const char *tgt) {
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
    die("invalid target: [%s]", tgt);
  }
}

static void stamp(sim_sb *args, char **&argp, const char *arg) {
  sim_sb_concat(args, " ");
  *argp++ = args->buffer + args->len;
  sim_sb_concat(args, arg);
}
static void add_sysroot(sim_sb *args, const char *target) {
  sim_sbt sra{};

  char *argv[10]{};
  char **argp = argv;

  *argp++ = sra.buffer;
  sim_sb_path_copy_parent(&sra, argv0);
  sim_sb_path_append(&sra, "leco-sysroot.exe");
  stamp(&sra, argp, "-t");
  stamp(&sra, argp, target);

  FILE *f;
  FILE *ferr;
  if (0 != proc_open(argv, &f, &ferr))
    die("could not infer sysroot");

  sim_sbt sysroot{};
  if (!fgets(sysroot.buffer, sysroot.size, f))
    die("failed to infer sysroot");

  sim_sbt buf{};
  while (!feof(ferr) && fgets(buf.buffer, buf.size, ferr) != nullptr) {
    fputs(buf.buffer, stderr);
  }

  fclose(f);
  fclose(ferr);
}

static void create_deplist(const char *out) {
  sim_sbt cmd{};
  sim_sb_path_copy_parent(&cmd, argv0);
  sim_sb_path_append(&cmd, "leco-deplist.exe");
  sim_sb_concat(&cmd, " -i ");
  sim_sb_concat(&cmd, out);
  sim_sb_path_set_extension(&cmd, "dag");
  run(cmd.buffer);
}

static void infer_output(sim_sb *args, const char *input, const char *target) {
  sim_sbt out{};

  auto ext = sim_path_extension(input);
  if (0 == strcmp(".cppm", ext)) {
    sim_sb_concat(args, " --precompile -o ");
    in2out(input, &out, "pcm", target);
  } else {
    sim_sb_concat(args, " -c -o ");
    in2out(input, &out, "o", target);
  }

  sim_sb_concat(args, out.buffer);

  if (0 == strcmp(".cppm", ext) || 0 == strcmp(".pcm", ext) ||
      0 == strcmp(".cpp", ext)) {
    create_deplist(out.buffer);

    sim_sb_concat(args, " @");
    sim_sb_concat(args, out.buffer);
    sim_sb_path_set_extension(args, "deps");
  }
}

int main(int argc, char **argv) try {
  argv0 = argv[0];

  struct gopt opts;
  GOPT(opts, argc, argv, "gi:Oo:t:v");

  bool debug{};
  bool opt{};
  bool cpp = true;
  bool verbose{};
  const char *target{HOST_TARGET};
  const char *input{};
  const char *output{};
  const char *ext{".cpp"};

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
    case 'o':
      output = val;
      break;
    case 'O':
      opt = true;
      break;
    case 't':
      target = val;
      break;
    case 'v':
      verbose = true;
      break;
    default:
      return usage();
    }
  }

  sim_sb args{};
  sim_sb_new(&args, 10240);
  if (cpp) {
    clang_cmd(&args, "clang++");
    if (0 != strcmp(ext, ".mm"))
      sim_sb_concat(&args, " -std=c++2b");
  } else {
    clang_cmd(&args, "clang");
    if (0 != strcmp(ext, ".m"))
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
  add_target_defs(&args, target);
  if (0 != strcmp(target, HOST_TARGET))
    add_sysroot(&args, target);

  if (input) {
    sim_sb_concat(&args, " ");
    sim_sb_concat(&args, input);
  }
  if (output) {
    sim_sb_concat(&args, " -o");
    sim_sb_concat(&args, output);
  } else if (input) {
    infer_output(&args, input, target);
  }

  for (auto i = 0; i < opts.argc; i++) {
    // TODO: escape argv
    sim_sb_concat(&args, " ");
    sim_sb_concat(&args, opts.argv[i]);
  }

  if (verbose) {
    fprintf(stderr, "%s\n", args.buffer);
  }

  run(args.buffer);
  return 0;
} catch (...) {
  return 1;
}
