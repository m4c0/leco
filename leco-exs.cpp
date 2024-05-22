#pragma leco tool

#define GOPT_IMPLEMENTATION

#include "die.hpp"
#include "gopt.hpp"

static void usage() { die("invalid usage"); }

int main(int argc, char **argv) try {
  const char *input{};
  const char *exedir{};

  auto opts = gopt_parse(argc, argv, "e:i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    case 'e':
      exedir = val;
      break;
    default:
      usage();
      break;
    }
  });
  if (!input || !exedir)
    usage();
  if (opts.argc != 0)
    usage();
} catch (...) {
  return 1;
}
