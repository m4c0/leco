#pragma leco tool

import popen;
import sys;

// TODO: investigate why changes in deps are not triggering these
// Example: change "sys.cppm", no tool gets its obj compiled

struct ctx {
  sim::sb src;
  sim::sb obj;
  sim::sb deps;
  sim::sb incs;
  const char * lang;
  p::proc * proc {};
};

static auto clang = sys::tool_cmd("clang");
static void * hs[8] {};
static ctx cs[8] {};

static void drain(ctx * c, int res) {
  auto &[pcm, obj, deps, incs, lang, proc] = *c;

  while (proc->gets())     err(proc->last_line_read());
  while (proc->gets_err()) err(proc->last_line_read());

  c->proc = {};
  if (res != 0) die("command failed: ", *clang, "-i", *c->src, "--", c->lang, "-c", "-o", *c->obj, *c->deps, *c->incs);
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

  ctx c {
    .src = sys::read_dag_tag('srcf', dag),
    .obj = sys::read_dag_tag('objf', dag),
  };
  if (calc_mtime(dag) < mtime::of(*c.obj)) return;

  auto ext = sim::path_extension(*c.src);
  c.lang = "-std=c++2b";
  if (ext == ".m" || ext == ".mm") c.lang = "-fmodules -fobjc-arc";
  else if (ext == ".c") c.lang = "-std=c11";
 
  c.deps = "@"_s + *(sim::path_parent(dag) / "deplist");
  c.incs = "@"_s + *(sim::path_parent(dag) / "includes");

  int i {};
  for (i = 0; i < 8; i++) if (hs[i] == nullptr) break;

  if (i == 8) {
    // TODO: "drain" buffers otherwise we might deadlock
    auto res = p::wait_any(hs, &i);
    drain(cs + i, res);
  }

  sys::log("compiling object", *c.obj);
  c.proc = new p::proc { *clang, "-i", *c.src, "--", c.lang, "-c", "-o", *c.obj, *c.deps, *c.incs };
  hs[i] = c.proc->handle();
  cs[i] = c;
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

