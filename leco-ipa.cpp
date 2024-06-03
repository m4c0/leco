#pragma leco tool
#define GOPT_IMPLEMENTATION

#include "die.hpp"
#include "gopt.hpp"

void usage() { die("invalid usage"); }

int main(int argc, char **argv) try {
  const char *input{};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      input = val;
      break;
    default:
      usage();
      break;
    }
  });
  if (!input || opts.argc != 0)
    usage();

  return 0;
} catch (...) {
  return 1;
}
