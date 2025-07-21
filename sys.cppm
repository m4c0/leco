module;
#define MCT_SYSCALL_IMPLEMENTATION
#include "../mct/mct-syscall.h"

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
#endif

export module sys;
export import mtime;
export import no;
export import popen;
export import print;
export import pprent;
export import sim;
export import strset;

export namespace sys {
inline void run(const char * cmd) {
  if (0 != system(cmd)) dief("command failed: %s", cmd);
}

__attribute__((format(printf, 1, 2))) inline void runf(const char * cmd, ...) {
  char buf[10240] {};

  va_list arg;
  va_start(arg, cmd);
  vsnprintf(buf, sizeof(buf), cmd, arg);
  va_end(arg);

  run(buf);
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

  auto msg = mct_syscall_link(src, dst);
  if (msg) die("error: ", msg);
}

using env = hay<
  char *,
  [](const char * name) {
    auto e = mct_syscall_dupenv(name);
    if (!e) die("missing environment variable: ", name);
    return e;
  },
  free>;

void mkdirs(const char *path) {
  if (0 == mct_syscall_mkdir(path)) return;
  if (errno == EEXIST) return;

  auto p = sim::path_parent(path);
  mkdirs(*p);

  mct_syscall_mkdir(path);
}

using file = hay<
  FILE *, 
  [](const char * name, const char * mode) {
    FILE * res = mct_syscall_fopen(name, mode);
    if (res == nullptr) die("could not open file: ", name);
    return res;
  },
  ::fclose>;

void dag_read(const char *dag, auto &&fn) try {
  file f { dag, "r" };

  char buf[10240];
  while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
    if (strlen(buf) < 5) die("invalid line in dag file: ", dag);

    uint32_t *id = reinterpret_cast<uint32_t *>(buf);
    char *file = reinterpret_cast<char *>(id + 1);
    file[strlen(file) - 1] = 0;

    fn(*id, file);
  }
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
void tool_run(const char * name) {
  run(*tool_cmd(name));
}
void tool_run(const char * name, const char * args, auto &&... as) {
  auto cmd = tool_cmd(name) + " ";
  cmd.printf(args, as...);
  run(*cmd);
}
void opt_tool_run(const char * name, auto &&... as) {
  if (mtime::of(*tool_cmd(name)) == 0) return;
  tool_run(name, as...);
}

const char * target() { 
  auto e = mct_syscall_dupenv("LECO_TARGET");
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

class mt {
  struct ctx {
    sim::sb cmd;
    sim::sb out;
    p::proc * proc {};
    void (*dtor)(const char *) = [](auto) {};
  };

  sim::sb m_clang = tool_cmd("clang");
  void * m_hs[8] {};
  ctx m_cs[8] {};

  void drain(ctx * c, int res) {
    auto &[cmd, out, proc, dtor] = *c;
  
    while (proc->gets())     errln(proc->last_line_read());
    while (proc->gets_err()) errln(proc->last_line_read());
  
    c->proc = {};
    if (res != 0) ::die("command failed: ", *cmd);
    (c->dtor)(*(c->out));
  }

  [[nodiscard]] auto reserve() {
    int i {};
    for (i = 0; i < 8; i++) if (m_hs[i] == nullptr) break;

    if (i == 8) {
      // TODO: "drain" buffers otherwise we might deadlock
      auto res = p::wait_any(m_hs, &i);
      drain(m_cs + i, res);
    }

    return i;
  }

public:
  ~mt() {
    for (auto i = 0; i < 8; i++) {
      if (!m_cs[i].proc) continue;
      drain(m_cs + i, m_cs[i].proc->wait());
    }
  }

  void run(const char * lbl, const char * out, ctx c) {
    auto i = reserve();
    sys::log(lbl, out);
    m_cs[i] = c;
    m_hs[i] = m_cs[i].proc->handle();
  }
};
} // namespace sys
