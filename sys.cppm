module;
#define SIM_IMPLEMENTATION
#include "sim.hpp"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#define _mkdir(x) mkdir((x), 0777)
#endif

export module sys;

export namespace sys {
struct death {};

__attribute__((format(printf, 1, 2))) inline void die(const char *msg, ...) {
  va_list arg;
  va_start(arg, msg);
  vfprintf(stderr, msg, arg);
  va_end(arg);

  fputs("\n", stderr);
  throw death{};
}
inline void run(const char *cmd) {
  if (0 != system(cmd))
    die("command failed: %s", cmd);
}

inline void log(const char *verb, const char *msg) {
  fprintf(stderr, "%20s %s\n", verb, msg);
}

void link(const char *src, const char *dst) {
  // TODO: remove if existing
#ifdef _WIN32
  DeleteFile(dst);
  if (!CreateHardLink(dst, src, nullptr))
    die("could not create hard-link");
#else
  ::unlink(dst);
  if (::link(src, dst) != 0) {
    perror("could not create hard-link");
    throw death{};
  }
#endif
}

const char *env(const char *name) {
#ifdef _WIN32
  char *buf;
  size_t sz;
  _dupenv_s(&buf, &sz, name);
  return buf;
#else
  return strdup(getenv(name));
#endif
}

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

FILE * fopen(const char * name, const char * mode) {
  FILE * res {};
#ifdef _WIN32
  if (0 != fopen_s(&res, name, mode)) die("could not open file [%s]", name);
#else
  res = ::fopen(name, mode);
  if (res == nullptr) die("could not open file [%s]", name);
#endif
  return res;
}
} // namespace sys
