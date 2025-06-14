#pragma leco tool

import popen;
import sys;

// TODO: investigate why changes in deps are not triggering these
// Example: change "sys.cppm", no tool gets its obj compiled

struct ctx {
  sim::sb pcm;
  sim::sb obj;
  sim::sb deps;
  p::proc * proc {};
};

static auto clang = sys::tool_cmd("clang");

static void drain(ctx * c, int res) {
  auto &[pcm, obj, deps, proc] = *c;

  while (proc->gets())     err(proc->last_line_read());
  while (proc->gets_err()) err(proc->last_line_read());

  c->proc = {};
  if (res != 0) die("command failed: ", *clang, "--", "-c", *pcm, "-o", *obj, *deps);
}

int main() try {
  static void * hs[8] {};
  static ctx cs[8] {};

  sys::for_each_tag_in_dags('pcmf' , true, [](auto * dag, auto file) {
    ctx c {
      .pcm { file },
      .obj = sys::read_dag_tag('objf', dag),
    };
    if (mtime::of(*c.pcm) < mtime::of(*c.obj)) return;

    c.deps = "@"_s + *(sim::path_parent(dag) / "deplist");

    int i {};
    for (i = 0; i < 8; i++) if (hs[i] == nullptr) break;

    if (i == 8) {
      auto res = p::wait_any(hs, &i);
      drain(cs + i, res);
    }

    sys::log("compiling mod obj", *c.obj);

    c.proc = new p::proc { *clang, "--", "-c", *c.pcm, "-o", *c.obj, *c.deps };
    hs[i] = c.proc->handle();
    cs[i] = c;
  });
  for (auto i = 0; i < 8; i++) {
    if (!cs[i].proc) continue;
    drain(cs + i, cs[i].proc->wait());
  }
} catch (...) {
  return 1;
}
