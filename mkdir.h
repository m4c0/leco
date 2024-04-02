#ifndef MKDIR_H
#define MKDIR_H

#ifdef __cplusplus
extern "C" {
#endif

void mkdirs(const char *path);

#ifdef __cplusplus
}
#endif
#endif // MKDIR_H

#ifdef MKDIR_IMPLEMENTATION

#include "pathmax.h"
#include "sim.h"
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#elif __linux__
#include <stdlib.h>
#define _mkdir(x) mkdir((x), 0777)
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
}

#endif // MKDIR_IMPLEMENTATION
