#pragma leco tool
#define GOPT_IMPLEMENTATION
#define MTIME_IMPLEMENTATION
#define SIM_IMPLEMENTATION
#define SYSSTD_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "../mtime/mtime.h"
#include "../sysstd/sysstd.h"
#include "sim.h"
#include "targets.hpp"

static const char *argv0;

static void clang_cmd(sim_sb *buf, const char *exe) {
#if __APPLE__ && !__arm64__
  sim_sb_copy(buf, "/usr/local/opt/llvm/bin");
  sim_sb_path_append(buf, exe);
#elif __APPLE__
  sim_sb_copy(buf, "/opt/homebrew/opt/llvm/bin");
  sim_sb_path_append(buf, exe);
#elif _WIN32
  sim_sb_copy(buf, exe);
  sim_sb_concat(buf, ".exe");
#else
  sim_sb_copy(buf, exe);
#endif
}

static void run(const char * cmd) {
  if (0 == system(cmd)) return;
  fprintf(stderr, "command failed: %s", cmd);
  throw 0;
}

static int usage() {
  fprintf(stderr, R"(
LECO's heavily-opiniated CLANG runner

Usage: %s [-i <input> [-o <output>]] [-t <target>] [-g] [-O] [-v] [-- <clang-flags>]

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
  if (IS_TGT(tgt, TGT_WINDOWS)) {
    sim_sb_concat(buf, " -DLECO_TARGET_WINDOWS");
  } else if (IS_TGT(tgt, TGT_LINUX)) {
    sim_sb_concat(buf, " -DLECO_TARGET_LINUX");
  } else if (IS_TGT(tgt, TGT_OSX)) {
    sim_sb_concat(buf, " -DLECO_TARGET_MACOSX");
    sim_sb_concat(buf, " -DLECO_TARGET_APPLE");
    sim_sb_concat(buf, " -D_C99_SOURCE");
  } else if (IS_TGT(tgt, TGT_IPHONEOS)) {
    sim_sb_concat(buf, " -DLECO_TARGET_IPHONEOS");
    sim_sb_concat(buf, " -DLECO_TARGET_IOS");
    sim_sb_concat(buf, " -DLECO_TARGET_APPLE");
    sim_sb_concat(buf, " -D_C99_SOURCE");
  } else if (IS_TGT(tgt, TGT_IOS_SIMULATOR)) {
    sim_sb_concat(buf, " -DLECO_TARGET_IPHONESIMULATOR");
    sim_sb_concat(buf, " -DLECO_TARGET_IOS");
    sim_sb_concat(buf, " -DLECO_TARGET_APPLE");
    sim_sb_concat(buf, " -D_C99_SOURCE");
  } else if (IS_TGT_DROID(tgt)) {
    sim_sb_concat(buf, " -DLECO_TARGET_ANDROID");
  } else if (IS_TGT(tgt, TGT_WASM)) {
    sim_sb_concat(buf, " -DLECO_TARGET_WASM");
  } else {
    fprintf(stderr, "invalid target: [%s]\n", tgt);
    throw 0;
  }
}

static void add_sysroot(sim_sb *args, const char *target) {
  sim_sb sra{};
  sim_sb_new(&sra, 10240);
  sim_sb_path_copy_parent(&sra, argv0);
  sim_sb_path_parent(&sra);
  sim_sb_path_append(&sra, target);
  sim_sb_path_append(&sra, "sysroot");

  if (mtime_of(sra.buffer) > 0) {
    auto f = sysstd_fopen(sra.buffer, "r");
    fgets(sra.buffer, sra.size, f);
    fclose(f);

    sim_sb_printf(args, " --sysroot %s", sra.buffer);
  }
}

static bool create_deplist(const char *out) {
  sim_sb dag{};
  sim_sb_new(&dag, 10240);
  sim_sb_copy(&dag, out);
  sim_sb_path_set_extension(&dag, "dag");
  if (mtime_of(dag.buffer) == 0)
    return false;

  sim_sb cmd{};
  sim_sb_new(&cmd, 10240);
  sim_sb_path_copy_parent(&cmd, argv0);
  sim_sb_path_append(&cmd, "leco-deplist.exe");
  if (mtime_of(cmd.buffer) == 0)
    return false;

  sim_sb_printf(&cmd, " -i %s -o %s", dag.buffer, dag.buffer);
  sim_sb_path_set_extension(&cmd, "deps");

  run(cmd.buffer);
  return true;
}

static void infer_output(sim_sb *args, const char *input, const char *target) {
  auto ext = sim_path_extension(input);

  sim_sb out{};
  sim_sb_new(&out, 10240);
  if (0 == strcmp(".pcm", ext)) {
    sim_sb_copy(&out, input);
  } else {
    sim_sb_path_copy_parent(&out, input);
    sim_sb_path_append(&out, "out");
    sysstd_mkdir(out.buffer);
    sim_sb_path_append(&out, target);
    sysstd_mkdir(out.buffer);
    sim_sb_path_append(&out, sim_path_filename(input));
  }

  if (0 == strcmp(".cppm", ext)) {
    sim_sb_concat(args, " --precompile -o ");
    sim_sb_path_set_extension(&out, "pcm");
  } else {
    sim_sb_concat(args, " -c -o ");
    sim_sb_path_set_extension(&out, "o");
  }

  sim_sb_concat(args, out.buffer);

  if (0 == strcmp(".cppm", ext) || 0 == strcmp(".pcm", ext) ||
      0 == strcmp(".cpp", ext)) {
    if (create_deplist(out.buffer)) {
      sim_sb_concat(args, " @");
      sim_sb_concat(args, out.buffer);
      sim_sb_path_set_extension(args, "deps");
    } else {
      sim_sb_concat(args, " -fprebuilt-module-path=out");
      sim_sb_path_append(args, target);
    }
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
  const char *output{};
  const char *ext{".cpp"};

  sim_sb input{};

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    case 'g':
      debug = true;
      break;
    case 'i': {
      sim_sb_new(&input, 10240);
      sim_sb_path_copy_real(&input, val);
      ext = sim_path_extension(input.buffer);
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

#ifdef __linux__
  if (cpp) {
    sim_sb_concat(&args, " -stdlib=libc++");
  }
#endif

  if (debug) {
#ifdef _WIN32
    sim_sb_concat(&args, " -gdwarf");
#else
    sim_sb_concat(&args, " -g");
#endif
  }
  if (opt) {
    sim_sb_concat(&args, " -O3 -flto -fvisibility=hidden");
  }

  sim_sb_printf(&args, " -target %s", target);
  add_target_defs(&args, target);
  if (0 != strcmp(target, HOST_TARGET)) add_sysroot(&args, target);

  if (input.len != 0) sim_sb_printf(&args, " %s", input.buffer);

  if (output) sim_sb_printf(&args, " -o %s", output);
  else if (input.len != 0) infer_output(&args, input.buffer, target);

  // TODO: escape argv
  for (auto i = 0; i < opts.argc; i++) sim_sb_printf(&args, " %s", opts.argv[i]);

  if (verbose) fprintf(stderr, "%s\n", args.buffer);

  run(args.buffer);
  return 0;
} catch (...) {
  return 1;
}
