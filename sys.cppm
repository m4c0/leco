module;
#define _CRT_NONSTDC_NO_DEPRECATE
#define MCT_SYSCALL_IMPLEMENTATION
#include "../mct/mct-syscall.h"

#include "targets.hpp"

#include <map>
#include <set>
#include <string>

export module sys;
export import c;
export import hay;
export import mtime;
export import no;
export import popen;
export import print;
export import pprent;
export import sim;
export import sv;

export namespace sys {
inline void run(const char * cmd) {
  if (0 != system(cmd)) dief("command failed: %s", cmd);
}

inline void runf(const char * cmd, auto ... args) {
  run(*sim::printf(cmd, args...));
}

inline void log(const char *verb, const char * msg) { errfn("%20s %s", verb, msg); }

void hardlink_switchroo(const char * f) {
#ifdef _WIN32
  // Rename original file. This is a "Windows-approved" way of modifying an open
  // executable or file.
  auto bkp = sim::sb { f } + ".bkp";
  remove(*bkp);
  rename(f, *bkp);
#elif __APPLE__
  // Apple's "hard-link" complains if the target file exist
  remove(f);
#endif
}

void link(const char *src, const char *dst) {
  if (mtime::of(dst) >= mtime::of(src)) return;
  sys::log("hard-linking", dst);

  hardlink_switchroo(dst);

  auto msg = mct_syscall_link(src, dst);
  if (msg) die("error: ", msg);
}

using file = hay<FILE *, &c::fopen, &c::fclose>;

//////////////////////////////////////////////////////////////////////////////
// Environment variables. Grabbing them all here for documentation purposes //
//////////////////////////////////////////////////////////////////////////////

using env = hay<
  char *,
  [](const char * name) {
    auto e = mct_syscall_dupenv(name);
    if (!e) die("missing environment variable: ", name);
    return e;
  },
  free>;

using opt_env = hay<char *, mct_syscall_dupenv, free>;

namespace envs {
  auto android_sdk_root() { return sys::env("ANDROID_SDK_ROOT"); }

  auto ios_api_key()          { return sys::env("LECO_IOS_API_KEY");    }
  auto ios_api_issuer()       { return sys::env("LECO_IOS_API_ISSUER"); }
  // Ad-hoc, development, app-store-connect
  auto ios_method()           { return sys::env("LECO_IOS_METHOD");     }
  auto ios_prov_profile()     { return sys::env("LECO_IOS_PROV_PROF");  }
  auto ios_sign_id()          { return sys::env("LECO_IOS_SIGN_ID");    }
  auto ios_simulator_target() { return sys::env("LECO_IOS_SIM_TARGET"); }
  auto ios_team_id()          { return sys::env("LECO_IOS_TEAM");       }
}
namespace opt_envs {
  auto debug() { return opt_env("LECO_DEBUG"); }
  auto opt()   { return opt_env("LECO_OPT");   }

  auto wasi_sysroot() { return opt_env("WASI_SYSROOT"); }
}

using target_env = hay<
  char *,
  [] {
    auto e = mct_syscall_dupenv("LECO_TARGET");
    return e ? e : strdup(HOST_TARGET);
  },
  free>;
auto target() { return target_env {}; }

bool is_debug() { return static_cast<const char *>(opt_envs::debug()); }
bool is_opt()   { return static_cast<const char *>(opt_envs::opt());   }

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

void for_each_target_def(auto && fn) {
  if (is_tgt_windows()) {
    fn("LECO_TARGET_WINDOWS");
  } else if (is_tgt_linux()) {
    fn("LECO_TARGET_LINUX");
  } else if (is_tgt_osx()) {
    fn("LECO_TARGET_MACOSX");
    fn("LECO_TARGET_APPLE");
  } else if (is_tgt_iphoneos()) {
    fn("LECO_TARGET_IPHONEOS");
    fn("LECO_TARGET_IOS");
    fn("LECO_TARGET_APPLE");
  } else if (is_tgt_ios_sim()) {
    fn("LECO_TARGET_IPHONESIMULATOR");
    fn("LECO_TARGET_IOS");
    fn("LECO_TARGET_APPLE");
  } else if (is_tgt_droid()) {
    fn("LECO_TARGET_ANDROID");
  } else if (is_tgt_wasm()) {
    fn("LECO_TARGET_WASM");
  } else {
    die("invalid target: ", (const char *)target());
  }
}

////////////////////////////////////////////////////////////////////////////

class strset {
  std::set<std::string> m_data{};

public:
  bool insert(std::string s) {
    auto [_, i] = m_data.insert(s);
    return i;
  }

  [[nodiscard]] auto begin() { return m_data.begin(); }
  [[nodiscard]] auto end() { return m_data.end(); }
};
class strmap {
  std::map<std::string, uint64_t> m_data {};

public:
  auto & operator[](std::string key) { return m_data[key]; }
};

void mkdirs(const char *path) {
  if (0 == mct_syscall_mkdir(path)) return;
  if (errno == EEXIST) return;

  auto p = sim::path_parent(path);
  mkdirs(*p);

  mct_syscall_mkdir(path);
}

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

void recurse_dag(strset * cache, const char * dag, auto && fn) {
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
  strset added {};
  recurse_dag(&added, dag, fn);
}

void for_each_dag(const char * basedir, bool recurse, auto && fn) {
  strset added {};
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
      case 'test':
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

#undef min
constexpr auto min(auto a, auto b) { return a > b ? b : a; }
#undef max
constexpr auto max(auto a, auto b) { return a > b ? a : b; }

class mt {
  using dtor_t = void (*)(const char *);
  struct ctx {
    sim::sb cmd;
    sim::sb out;
    p::proc * proc {};
    dtor_t dtor = [](auto) {};
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

  [[nodiscard]] int reserve() {
    int i {};
    for (i = 0; i < 8; i++) if (m_hs[i] == nullptr) break;

    if (i == 8) {
      // TODO: "drain" buffers otherwise we might deadlock
      auto res = p::wait_any(m_hs, &i);
      if (res == -1) {
        errln("could not wait for child process");
        // Try again, worst case we crash on infinite recursion
        return reserve();
      }
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
  void run_clang(const char * lbl, const char * out, dtor_t dtor, auto *... args) {
    run(lbl, out, ctx {
      .cmd = (m_clang + ... + args),
      .out { out },
      .proc = new p::proc { *m_clang, args... },
      .dtor = dtor,
    });
  }
  void run_clang(const char * lbl, const char * out, auto *... args) {
    run_clang(lbl, out, [](auto) {}, args...);
  }
};
} // namespace sys
