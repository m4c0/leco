#include "bouncer.hpp"
#include "cl.hpp"
#include "dag.hpp"

#define MINIRENT_IMPLEMENTATION
#include "../minirent/minirent.h"

using namespace llvm;

static bool error() {
  fprintf(stderr, "Could not list current directory: %s\n", strerror(errno));
  return false;
}

bool run_target() {
  dag::clear_cache();

  DIR *dir = opendir(".");
  if (dir == nullptr)
    return error();

  struct dirent *dp{};
  while ((dp = readdir(dir))) {
    auto nm = dp->d_name;
    if (!bounce(nm)) {
      return false;
    }

    errno = 0;
  }
  if (errno)
    return error();

  closedir(dir);
  return true;
}

extern "C" int main(int argc, char **argv) {
  if (!parse_args(argc, argv))
    return 1;

  try {
    return for_each_target(run_target) ? 0 : 1;
  } catch (const std::exception &e) {
    fprintf(stderr, "exception: %s\n", e.what());
    return 1;
  } catch (...) {
    fprintf(stderr, "unexpected exception\n");
    return 1;
  }
}
