#pragma leco tool

import popen;
import sys;

// TODO: investigate why changes in deps are not triggering these
// Example: change "sys.cppm", no tool gets its obj compiled

struct ctx {
  sim::sb cmd;
  p::proc * proc {};
};

static auto clang = sys::tool_cmd("clang");
static void * hs[8] {};
static ctx cs[8] {};

static void drain(ctx * c, int res) {
  auto &[cmd, proc] = *c;

  while (proc->gets())     errln(proc->last_line_read());
  while (proc->gets_err()) errln(proc->last_line_read());

  c->proc = {};
  if (res != 0) die("command failed: ", *cmd);
}

static str::map spec_cache {};
static auto calc_mtime(const char * dag) {
  auto &mtime = spec_cache[dag];
  if (mtime != 0) return mtime;
  mtime = mtime::of(dag);

  sys::dag_read(dag, [&](auto id, auto file) {
    switch (id) {
      case 'head': mtime = sys::max(mtime, mtime::of(file)); break;
      case 'mdag': mtime = sys::max(mtime, calc_mtime(file)); break;
    }
  });

  return mtime;
}

static void compile_objf(const char * dag, const char * _) {
  if (sys::read_dag_tag('pcmf', dag) != "") return;

  auto src = sys::read_dag_tag('srcf', dag);
  auto obj = sys::read_dag_tag('objf', dag);
  if (calc_mtime(dag) < mtime::of(*obj)) return;

  auto ext = sim::path_extension(*src);
  auto lang = "-std=c++2b";
  if (ext == ".m" || ext == ".mm") lang = "-fmodules -fobjc-arc";
  else if (ext == ".c") lang = "-std=c11";
 
  auto deps = "@"_s + *(sim::path_parent(dag) / "deplist");
  auto incs = "@"_s + *(sim::path_parent(dag) / "includes");

  int i {};
  for (i = 0; i < 8; i++) if (hs[i] == nullptr) break;

  if (i == 8) {
    // TODO: "drain" buffers otherwise we might deadlock
    auto res = p::wait_any(hs, &i);
    drain(cs + i, res);
  }

  sys::log("compiling object", *obj);
  cs[i] = {
    .cmd = clang + " -i " + *src + " -- " + lang + " -c -o " + *obj + " " + *deps + " " + *incs,
    .proc = new p::proc { *clang, "-i", *src, "--", lang, "-c", "-o", *obj, *deps, *incs },
  };
  hs[i] = cs[i].proc->handle();
}

int main() try {
  sys::for_each_tag_in_dags('objf', true, &compile_objf);

  for (auto i = 0; i < 8; i++) {
    if (!cs[i].proc) continue;
    drain(cs + i, cs[i].proc->wait());
  }
} catch (...) {
  return 1;
}

