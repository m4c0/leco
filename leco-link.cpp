#pragma leco tool

#include <stdio.h>

import popen;
import sys;

struct ctx {
  sim::sb cmd;
  sim::sb exe;
  p::proc * proc {};
};

static const auto clang = sys::tool_cmd("clang");
static void * hs[8] {};
static ctx cs[8] {};

static void drain(ctx * c, int res) {
  auto &[cmd, exe, proc] = *c;

  while (proc->gets())     err(proc->last_line_read());
  while (proc->gets_err()) err(proc->last_line_read());

  c->proc = {};
  if (0 != res) die("command failed: ", *c->cmd);

#ifdef _WIN32
  auto old_f = exe + ".old";
  auto new_f = exe + ".new";
  rename(*exe, *old_f);
  rename(*new_f, *exe);
#endif
}

static void put_escape(sys::file & out, const char *a) {
  while (*a != 0) {
    char c = *a++;
    if (c == '\\') fput(out, "\\\\"); // escapes backslash
    else fput(out, c);
  }
  fputln(out);
}

static void add_local_fw(sys::file & out, const char * fw) {
  auto stem = sim::path_stem(fw);
  auto path = sim::path_parent(fw);

  fputfn(out, "-F%s\n-framework\n%s", *path, stem.buffer);
}

static void read_dag(str::map & cache, const char * dag, sys::file & out) {
  auto & mtime = cache[dag];
  if (mtime != 0) return;
  mtime = 1;

  sim::sb obj {};
  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
    case 'tdll': fputfn(out, "-shared"); break;
    case 'frwk': fputfn(out, "-framework\n%s", file); break;
    case 'libr': fputfn(out, "-l%s", file); break;
    case 'ldir': fputfn(out, "-L%s", file); break;
    case 'rpth': fputfn(out, "-Wl,-rpath,%s", file); break;
    case 'slib': fputfn(out, "%s", file); break;
    case 'objf': obj = sim::sb { file }; break;
    case 'xcfw': add_local_fw(out, file); break;
    case 'idag':
    case 'mdag': read_dag(cache, file, out); break;
    default: break;
    }
  });

  // Add object after dependencies as this kinda enforces the static init order
  put_escape(out, *obj);
}

static auto mtime_of(const char * exe_dag) {
  mtime::t res {};
  sys::recurse_dag(exe_dag, [&](auto dag, auto id, auto file) {
    if (id == 'objf') res = sys::max(res, mtime::of(file));
  });
  return res;
}

static void prepare_args(const char * input, const char * args) {
  sys::file out { args, "wb" };
  str::map cache {};
  read_dag(cache, input, out);

  if (sys::is_tgt_osx()) {
    // Required for custom frameworks
    fputln(out, "-rpath\n@executable_path/../Frameworks");
    // Useful for third-party dylibs, like vulkan loader
    fputln(out, "-rpath\n@executable_path");
  } else if (sys::is_tgt_ios()) {
    fputln(out, "-rpath\n@executable_path/Frameworks");
  } else if (sys::is_tgt_windows()) {
    // otherwise, face LNK1107 errors from MSVC
    fputln(out, "-fuse-ld=lld");

    auto rc = sim::sb { input }.path_extension("res");
    if (mtime::of(*rc) > 0) fputln(out, *rc);
  } else if (sys::is_tgt_wasm()) {
    char sra[1024] {};

    fgets(sra, sizeof(sra) - 1, sys::file { "../leco/out/wasm32-wasi/sysroot", "r" });
    fputln(out, "-resource-dir\n", sra);

    // export-table: allows passing lambdas to JS
    fputln(out, "-Xlinker\n--export-table");
    // exec-model: without it, we could use "main" but _start calls the global
    //             dtor after "main" returns
    fputln(out, "-mexec-model=reactor");
    // no-check-features: allows using shared-memory without atomics/etc
    //                    https://stackoverflow.com/a/70186219
    //                    Only works because we won't be using malloc in threads
    // cmd += " -Xlinker --shared-memory -Xlinker --no-check-features";
  }
}
void run(const char * input, const char * output) {
  if (mtime_of(input) <= mtime::of(output)) return;

  sim::sb args { input };
  args.path_extension("link");
  prepare_args(input, *args);

#ifdef _WIN32
  // We can rename but we can't overwrite an open executable.
  // To link "clang" we need the old version, so we have to compile in a new
  // place then do the appropriate renames.

  auto next = sim::sb { output } + ".new";
  remove(*next);
  const char * exe = *next;
#else
  const char * exe = output;
#endif

  int i {};
  for (i = 0; i < 8; i++) if (hs[i] == nullptr) break;

  if (i == 8) {
    // TODO: "drain" buffers otherwise we might deadlock
    auto res = p::wait_any(hs, &i);
    drain(cs + i, res);
  }

  auto a = "@"_s + *args;

  sys::log("linking", output);
  cs[i] = {
    .cmd = clang + " -- " + *a + " -o " + exe,
    .exe = sim::sb { output },
    .proc = new p::proc { *clang, "--", *a, "-o", exe },
  };
  hs[i] = cs[i].proc->handle();
}

int main() try {
  sys::for_each_root_dag([](auto * dag, auto id, auto file) {
    if (id != 'tmmd') run(dag, file);
  });

  for (auto i = 0; i < 8; i++) {
    if (!cs[i].proc) continue;
    drain(cs + i, cs[i].proc->wait());
  }

  return 0;
} catch (...) {
  return (fprintf(stderr, "err\n"), 1);
}
