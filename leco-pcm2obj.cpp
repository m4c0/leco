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

int main() try {
  sys::for_each_tag_in_dags('pcmf' , true, [](auto * dag, auto file) {
    auto pcm = file;
    auto obj = sys::read_dag_tag('objf', dag);
    if (mtime::of(pcm) < mtime::of(*obj)) return;

    auto deps = "@"_s + *(sim::path_parent(dag) / "deplist");

    int i {};
    for (i = 0; i < 8; i++) if (hs[i] == nullptr) break;

    if (i == 8) {
      // TODO: "drain" buffers otherwise we might deadlock
      auto res = p::wait_any(hs, &i);
      drain(cs + i, res);
    }

    sys::log("compiling mod obj", *obj);

    cs[i] = {
      .cmd = clang + " -- -c " + pcm + " -o " + *obj + " " + *deps,
      .proc = new p::proc { *clang, "--", "-c", pcm, "-o", *obj, *deps },
    };
    hs[i] = cs[i].proc->handle();
  });
  for (auto i = 0; i < 8; i++) {
    if (!cs[i].proc) continue;
    drain(cs + i, cs[i].proc->wait());
  }
} catch (...) {
  return 1;
}
