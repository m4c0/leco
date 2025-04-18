module;
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
#include <windows.h>
#else
#include <unistd.h>
#endif

export module sys;
import mtime;
import pprent;
import sim;
import strset;
import sysstd;

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

void unlink(const char * f) {
#ifdef _WIN32
  DeleteFile(f);
#else
  ::unlink(f);
  rmdir(f);
#endif
}

void link(const char *src, const char *dst) {
  unlink(dst);
#ifdef _WIN32
  if (!CreateHardLink(dst, src, nullptr))
    die("could not create hard-link");
#elif __APPLE__
  if (0 != clonefile(src, dst, 0)) die("could not clone file");
#else
  if (::link(src, dst) != 0) {
    perror("could not create hard-link");
    throw death{};
  }
#endif
}

const char *env(const char *name) {
  auto e = sysstd::env(name);
  if (e) return e;
  sys::die("missing environment variable [%s]", name);
}

void mkdirs(const char *path) {
  if (0 == sysstd::mkdir(path)) return;
  if (errno == EEXIST) return;

  auto p = sim::path_parent(path);
  mkdirs(*p);

  sysstd::mkdir(path);
}

FILE * fopen(const char * name, const char * mode) {
  FILE * res = sysstd::fopen(name, mode);
  if (res == nullptr) die("could not open file [%s]", name);
  return res;
}
void fclose(FILE * f) { ::fclose(f); }

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

  ::fclose(f);
} catch (...) {
  fprintf(stderr, "whilst reading DAG node [%s]\n", dag);
  throw;
}

void recurse_dag(str::set * cache, const char * dag, auto && fn) {
  if (!cache->insert(dag)) return;

  dag_read(dag, [&](auto id, auto file) {
    fn(dag, id, file);
    switch (id) {
      case 'idag':
      case 'mdag': recurse_dag(cache, file, fn); break;
      default: break;
    }
  });
}
void recurse_dag(const char * dag, auto && fn) {
  str::set added {};
  recurse_dag(&added, dag, fn);
}

void for_each_dag(const char * target, bool recurse, auto && fn) {
  auto path = ("out"_real /= target);
  for (auto file : pprent::list(*path)) {
    if (sim::path_extension(file) != ".dag") continue;

    auto dag = path / file;
    if (recurse) {
      recurse_dag(*dag, fn);
    } else {
      dag_read(*dag, [&](auto id, auto file) {
        fn(*dag, id, file);
      });
    }
  }
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
void opt_tool_run(const char * name) {
  auto cmd = tool_cmd(name);
  if (mtime::of(*cmd) == 0) return;
  run(*(cmd + " "));
}
void opt_tool_run(const char * name, const char * args, auto &&... as) {
  auto cmd = tool_cmd(name);
  if (mtime::of(*cmd) == 0) return;
  run(*(cmd + " ").printf(args, as...));
}

const char * target() { return env("LECO_TARGET"); }

constexpr const char * host_target = HOST_TARGET;
bool is_tgt(const char * t, const char * x) { return 0 == strcmp(t, x); }
bool is_tgt_host    (const char * t) { return is_tgt(t, HOST_TARGET); }
bool is_tgt_linux   (const char * t) { return is_tgt(t, TGT_LINUX); }
bool is_tgt_wasm    (const char * t) { return is_tgt(t, TGT_WASM); }
bool is_tgt_windows (const char * t) { return is_tgt(t, TGT_WINDOWS); }
bool is_tgt_iphoneos(const char * t) { return is_tgt(t, TGT_IPHONEOS); }
bool is_tgt_ios_sim (const char * t) { return is_tgt(t, TGT_IOS_SIMULATOR); }
bool is_tgt_osx     (const char * t) { return is_tgt(t, TGT_OSX); }
bool is_tgt_droid(const char * t) {
  return is_tgt(t, TGT_DROID_AARCH64) || is_tgt(t, TGT_DROID_ARMV7)
      || is_tgt(t, TGT_DROID_X86) || is_tgt(t, TGT_DROID_X86_64);
}
bool is_tgt_ios(const char * t) { return is_tgt_iphoneos(t) || is_tgt_ios_sim(t); }
bool is_tgt_apple(const char * t) { return is_tgt_osx(t) || is_tgt_ios(t); }
} // namespace sys
