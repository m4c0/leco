module;
#define SIM_IMPLEMENTATION
#include "sim.hpp"
#include "targets.hpp"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <sys/clonefile.h>
#endif

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
import sim;

export namespace sys {
struct death {};

[[noreturn]] __attribute__((format(printf, 1, 2))) inline void die(const char *msg, ...) {
  va_list arg;
  va_start(arg, msg);
  vfprintf(stderr, msg, arg);
  va_end(arg);

  fputs("\n", stderr);
  throw death{};
}
inline void run(const char *cmd) {
  if (0 != system(cmd)) die("command failed: %s", cmd);
}
__attribute__((format(printf, 1, 2))) inline void runf(const char * cmd, ...) {
  char buf[10240] {};

  va_list arg;
  va_start(arg, cmd);
  vsnprintf(buf, sizeof(buf), cmd, arg);
  va_end(arg);

  run(buf);
}

inline void log(const char *verb, const char *msg) {
  fprintf(stderr, "%20s %s\n", verb, msg);
}

void link(const char *src, const char *dst) {
#ifdef _WIN32
  DeleteFile(dst);
  if (!CreateHardLink(dst, src, nullptr))
    die("could not create hard-link");
#elif __APPLE__
  ::unlink(dst);
  if (0 != clonefile(src, dst, 0)) die("could not clone file");
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
  if (0 == _dupenv_s(&buf, &sz, name)) return buf;
#else
  auto e = getenv(name);
  if (e) return strdup(e);
#endif
  sys::die("missing environment variable [%s]", name);
}

void mkdirs(const char *path) {
  if (0 == _mkdir(path))
    return;
  if (errno == EEXIST)
    return;

  auto p = sim::path_parent(path);
  mkdirs(*p);

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

void dag_read(const char *dag, auto &&fn) try {
  auto f = fopen(dag, "r");
  if (!f) die("could not open dag file");

  char buf[10240];
  while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
    if (strlen(buf) < 5)
      die("invalid line in dag file [%s]", dag);

    uint32_t *id = reinterpret_cast<uint32_t *>(buf);
    char *file = reinterpret_cast<char *>(id + 1);
    file[strlen(file) - 1] = 0;

    fn(*id, file);
  }

  fclose(f);
} catch (...) {
  fprintf(stderr, "whilst reading DAG node [%s]\n", dag);
  throw;
}

auto tool_cmd(const char * name) {
  return (sim::path_real("../leco/out/" HOST_TARGET) / "leco-") + name + ".exe";
}
auto tool_cmd(const char * name, const char * args, auto &&... as) {
  auto cmd = tool_cmd(name) + " ";
  return cmd.printf(args, as...);
}
void tool_run(const char * name) {
  auto cmd = tool_cmd(name);
  run(*cmd);
}
void tool_run(const char * name, const char * args, auto &&... as) {
  auto cmd = tool_cmd(name, args, as...);
  run(*cmd);
}
} // namespace sys
