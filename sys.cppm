module;
#include "../hay/hay.hpp"
#include "targets.hpp"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
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
export import mtime;
export import no;
export import print;
export import pprent;
export import sim;
export import strset;
export import sysstd;

export namespace sys {
__attribute__((format(printf, 1, 2))) inline void runf(const char * cmd, ...) {
  char buf[10240] {};

  va_list arg;
  va_start(arg, cmd);
  vsnprintf(buf, sizeof(buf), cmd, arg);
  va_end(arg);

  if (0 != system(buf)) dief("command failed: %s", buf);
}

inline void log(const char *verb, const char * msg) { errfn("%20s %s", verb, msg); }

constexpr const auto remove = ::remove;

void link(const char *src, const char *dst) {
  if (mtime::of(dst) >= mtime::of(src)) return;
  sys::log("hard-linking", dst);

  if (0 != remove(dst)) {
    // Rename original file. This is a "Windows-approved" way of modifying an
    // open executable or file.
    auto bkp = sim::sb { dst } + ".bkp";
    remove(*bkp);
    rename(dst, *bkp);
  }

  auto msg = sysstd::link(src, dst);
  if (msg) die("error: ", msg);
}

const char *env(const char *name) {
  auto e = sysstd::env(name);
  if (e) return e;
  die("missing environment variable: ", name);
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
  if (res == nullptr) die("could not open file: ", name);
  return res;
}
void fclose(FILE * f) { ::fclose(f); }

using file = hay<FILE *, sys::fopen, ::fclose>;

void dag_read(const char *dag, auto &&fn) try {
  auto f = fopen(dag, "r");
  if (!f) die("could not open dag file");

  char buf[10240];
  while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
    if (strlen(buf) < 5) die("invalid line in dag file: ", dag);

    uint32_t *id = reinterpret_cast<uint32_t *>(buf);
    char *file = reinterpret_cast<char *>(id + 1);
    file[strlen(file) - 1] = 0;

    fn(*id, file);
  }

  ::fclose(f);
} catch (...) {
  whilst("reading DAG node: ", dag);
}
auto read_dag_tag(uint32_t tid, const char * dag) {
  sim::sb res {};
  dag_read(dag, [&](auto id, auto file) {
    if (id == tid) res = sim::sb { file };
  });
  return res;
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

const char * target(); 
void for_each_dag(const char * basedir, bool recurse, auto && fn) {
  str::set added {};
  auto path = (sim::sb { basedir } /= "out") /= target();
  for (auto file : pprent::list(*path)) {
    if (sim::path_extension(file) != ".dag") continue;

    auto dag = path / file;
    if (recurse) {
      recurse_dag(&added, *dag, fn);
    } else {
      dag_read(*dag, [&](auto id, auto file) {
        fn(*dag, id, file);
      });
    }
  }
}
void for_each_dag(bool recurse, auto && fn) {
  for_each_dag(*"."_real, recurse, fn);
}
void for_each_root_dag(auto && fn) {
  for_each_dag(false, [&](auto dag, auto id, auto file) {
    switch (id) {
      case 'tapp':
      case 'tdll':
      case 'tool':
      case 'tmmd': fn(dag, id, file); break;
      default: break;
    }
  });
}

void for_each_tag_in_dags(auto tid, bool recurse, auto && fn) {
  for_each_dag(recurse, [&](auto dag, auto id, auto file) {
    if (id == tid) fn(dag, file);
  });
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
  runf("%s", *cmd);
}
void tool_run(const char * name, const char * args, auto &&... as) {
  auto cmd = tool_cmd(name, args, as...);
  runf("%s", *cmd);
}
void opt_tool_run(const char * name) {
  auto cmd = tool_cmd(name);
  if (mtime::of(*cmd) == 0) return;
  runf("%s", *cmd);
}
void opt_tool_run(const char * name, const char * args, auto &&... as) {
  auto cmd = tool_cmd(name);
  if (mtime::of(*cmd) == 0) return;
  runf("%s", *(cmd + " ").printf(args, as...));
}

const char * target() { 
  auto e = sysstd::env("LECO_TARGET");
  return e ? e : HOST_TARGET;
}

constexpr const char * host_target = HOST_TARGET;
bool is_tgt(const char * x) { return 0 == strcmp(target(), x); }
bool is_tgt_host()     { return is_tgt(HOST_TARGET); }
bool is_tgt_linux()    { return is_tgt(TGT_LINUX); }
bool is_tgt_wasm()     { return is_tgt(TGT_WASM); }
bool is_tgt_windows()  { return is_tgt(TGT_WINDOWS); }
bool is_tgt_iphoneos() { return is_tgt(TGT_IPHONEOS); }
bool is_tgt_ios_sim()  { return is_tgt(TGT_IOS_SIMULATOR); }
bool is_tgt_osx()      { return is_tgt(TGT_OSX); }
bool is_tgt_droid() {
  return is_tgt(TGT_DROID_AARCH64) || is_tgt(TGT_DROID_ARMV7)
      || is_tgt(TGT_DROID_X86)     || is_tgt(TGT_DROID_X86_64);
}
bool is_tgt_ios()   { return is_tgt_iphoneos() || is_tgt_ios_sim(); }
bool is_tgt_apple() { return is_tgt_osx()      || is_tgt_ios();     }

#pragma clang diagnostic ignored "-Wgcc-compat"
[[noreturn]] __attribute__((format(printf, 1, 2))) inline void die(const char *msg, auto &&... args) {
  ::dief(msg, args...);
}

#undef max
constexpr auto max(auto a, auto b) { return a > b ? a : b; }
} // namespace sys
