#pragma leco tool

import c42;
import hai;
import jojo;
import jute;
import popen;
import sys;

// Compiles shaders referenced by their modules via pragmas.
// 
// Requires DAGs created via leco-dagger.

using namespace c;

static void build_shader(const char * dag, const char * file) {
  auto out = sim::path_parent(dag) / sim::path_filename(file) + ".spv";
  auto spv_time = mtime::of(*out);
  bool stale = mtime::of(file) > spv_time;

  auto fsv = sv::unsafe(file);
  auto stem = fsv.rsplit('.').after;

  const char * argv[1024] {};
  int argc = 0;

  argv[argc++] = "glslangValidator";
  argv[argc++] = "--target-env";
  argv[argc++] = "spirv1.3";
  argv[argc++] = "-V";
  argv[argc++] = "-S";
  argv[argc++] = stem.begin();
  argv[argc++] = "-e";
  argv[argc++] = "main";
  argv[argc++] = "-o";
  argv[argc++] = *out;
  argv[argc++] = file;

  struct : c42::defines {
    bool has(sv name) const override { return false; }
  } d {};
  auto src = jojo::read_cstr(fsv);
  auto ctx = c42::preprocess(&d, src);
  for (auto t : ctx) {
    switch (t.type) {
      case c42::t_pragma: {
        auto [l, r] = ctx.txt(t).split(' ');
        if (l != "leco") continue;

        auto [cmd, param] = r.trim().split(' ');
        if (cmd == "include") {
          auto file = hai::cstr { param.trim().split('"').after.split('"').before };
          auto real = sim::path_real(file.begin());
          argv[argc++] = strndup(real.buffer, real.len);
          stale |= mtime::of(*real) > spv_time;
        }
        break;
      }
      default: break;
    }
  }
  if (!stale) return;

  sys::log("compiling shader", file);
  p::proc p { argc, argv };

  while (p.gets()) {
    auto line = jute::view::unsafe(p.last_line_read());
    if (line.starts_with("ERROR: ")) {
      if (line[7] == '/') line = line.subview(7).after;
      else if (line[8] == ':') line = line.subview(7).after;
    }
    errln(line);
  }

  if (p.wait() != 0) die("shader compilation failed");

  if (!sys::is_tgt_wasm()) return;

  auto gles = sim::path_parent(dag) / sim::path_filename(file) + ".gles";
  sys::runf("spirv-cross --es --version 300 --output %s %s", *gles, *out);
}

static void run(const char * dag, const char * _) {
  sys::recurse_dag(dag, [&](auto dag, auto id, auto file) {
    if (id == 'shdr') build_shader(dag, file);
  });
}

int main() try {
  sys::for_each_tag_in_dags('tapp', false, &run);
  sys::for_each_tag_in_dags('tool', false, &run);
} catch (...) {
  return 1;
}
