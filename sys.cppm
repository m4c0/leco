module;
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#else
#include <unistd.h>
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
} // namespace sys
