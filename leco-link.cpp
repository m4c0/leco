#pragma leco tool
#define GOPT_IMPLEMENTATION
#define SIM_IMPLEMENTATION

#include "dag2.hpp"
#include "die.hpp"
#include "fopen.hpp"
#include "gopt.hpp"
#include "in2out.hpp"
#include "sim.hpp"

#include <set>
#include <string>

static const char *target{};
static FILE *out{};
static std::set<std::string> added{};

static void usage() { die("invalid usage"); }

static void put(const char *a) {
  while (*a != 0) {
    char c = *a++;
    if (c == '\\') {
      fputs("\\\\", out); // escapes backslash
    } else {
      fputc(c, out);
    }
  }
  fputc('\n', out);
}

static void read_dag(const char *dag) {
  auto [_, inserted] = added.insert(dag);
  if (!inserted)
    return;

  dag_read(dag, [](auto id, auto file) {
    switch (id) {
    case 'tdll':
      fprintf(out, "-shared\n");
      break;
    case 'frwk':
      fprintf(out, "-framework\n%s\n", file);
      break;
    case 'libr':
      fprintf(out, "-l%s\n", file);
      break;
    case 'ldir':
      fprintf(out, "-L%s\n", file);
      break;
    case 'impl':
    case 'mdep': {
      sim_sbt ddag{};
      in2out(file, &ddag, "dag", target);
      read_dag(ddag.buffer);
      break;
    }
    default:
      break;
    }
  });

  // Add object after dependencies as this kinda enforces the static init order
  sim_sbt obj{};
  sim_sb_copy(&obj, dag);
  sim_sb_path_set_extension(&obj, "o");
  put(obj.buffer);
}

int main(int argc, char **argv) try {
  const char *input{};
  const char *output{};
  bool debug{};
  bool opt{};
  bool verbose{};
  auto opts = gopt_parse(argc, argv, "i:o:gOv", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    case 'o':
      output = val;
      break;
    case 'g':
      debug = true;
      break;
    case 'O':
      opt = true;
      break;
    case 'v':
      verbose = true;
      break;
    default:
      usage();
    }
  });
  if (!input || !output)
    usage();

  sim_sbt path{};
  sim_sb_path_copy_parent(&path, input);
  target = sim_sb_path_filename(&path);

  sim_sbt args{};
  sim_sb_copy(&args, input);
  sim_sb_path_set_extension(&args, "link");

  {
    f::open f{args.buffer, "wb"};
    out = *f;

    for (auto i = 0; i < opts.argc; i++) {
      fprintf(out, "%s\n", opts.argv[i]);
    }

    read_dag(input);
  }

#ifdef _WIN32
  // We can rename but we can't overwrite an open executable.
  // To link "clang" we need the old version, so we have to compile in a new
  // place then do the appropriate renames.

  sim_sbt next{};
  sim_sb_copy(&next, output);
  sim_sb_concat(&next, ".new");

  sim_sbt prev{};
  sim_sb_copy(&prev, output);
  sim_sb_concat(&prev, ".old");

  remove(next.buffer);
  remove(prev.buffer);
#endif

  sim_sbt cmd{};
  sim_sb_path_copy_parent(&cmd, argv[0]);
  sim_sb_path_append(&cmd, "leco-clang.exe");
  if (debug)
    sim_sb_concat(&cmd, " -g");
  if (opt)
    sim_sb_concat(&cmd, " -O");
  sim_sb_concat(&cmd, " -- @");
  sim_sb_concat(&cmd, args.buffer);
  sim_sb_concat(&cmd, " -o ");
#ifdef _WIN32
  sim_sb_concat(&cmd, next.buffer);
  // otherwise, face LNK1107 errors from MSVC
  sim_sb_concat(&cmd, " -fuse-ld=lld");
#else
  sim_sb_concat(&cmd, output);
#endif

  if (verbose)
    fprintf(stderr, "%s\n", cmd.buffer);

  run(cmd.buffer);

#ifdef _WIN32
  rename(output, prev.buffer);
  rename(next.buffer, output);
#endif

  return 0;
} catch (...) {
  return 1;
}
