#define GOPT_IMPLEMENTATION
#include "../gopt/gopt.h"

static auto gopt_parse(int argc, char **argv, const char *format, auto &&fn) {
  struct gopt opts;
  GOPT(opts, argc, argv, format);

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    fn(ch, val);
  }
  return opts;
}
