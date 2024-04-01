#include "mkdir.hpp"

#include "sim.hpp"
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <stdlib.h>
#define _mkdir(x) mkdir((x), 0777);
#endif

void mkdirs(const char *path) {
  if (0 == _mkdir(path))
    return;
  if (errno == EEXIST)
    return;

  sim_sbt p{};
  sim_sb_path_copy_parent(&p, path);
  mkdirs(p.buffer);
}
