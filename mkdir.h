#pragma once
#include "pathmax.h"
#include "sim.h"

#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#define _mkdir(x) mkdir((x), 0777)
#endif

void mkdirs(const char *path) {
  if (0 == _mkdir(path))
    return;
  if (errno == EEXIST)
    return;

  sim_sb p = {0};
  sim_sb_new(&p, PATH_MAX);
  sim_sb_path_copy_parent(&p, path);
  mkdirs(p.buffer);
  sim_sb_delete(&p);

  _mkdir(path);
}
