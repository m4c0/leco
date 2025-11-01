#pragma leco tool

import glen;
import jojo;
import jute;
import popen;
import sys;

// Compiles shaders referenced by their modules via pragmas.
// 
// Requires DAGs created via leco-dagger.

static glen::parser parser { tree_sitter_glsl };

static bool must_recompile(const char * file, auto spv_time) {
  if (spv_time < mtime::of(file)) return true;

  bool result = false;
  auto src = jojo::read_cstr(jute::view::unsafe(file));
  parser.parse(src).for_each_capture(jute::view { R"(
    (preproc_include (string_literal (string_content) @inc))
  )" }, [&](auto & n) {
    auto s = ts_node_start_byte(n);
    auto e = ts_node_end_byte(n);
    auto path = jute::view { src.begin() + s, e - s }.cstr();
    if (!mtime::of(path.begin())) {
      auto [l, c] = ts_node_start_point(n);
      dief("%s:%d:%d: include file not found", file, l, c);
    }
    if (must_recompile(path.begin(), spv_time)) result = true;
  });

  return result;
}
static void build_shader(const char * dag, const char * file) {
  auto out = sim::path_parent(dag) / sim::path_filename(file) + ".spv";
  if (!must_recompile(file, mtime::of(*out))) return;

  sys::log("compiling shader", file);
  p::proc p { "glslangValidator", "--target-env", "spirv1.3", "-V", "-o", *out, file };

  while (p.gets()) {
    auto line = jute::view::unsafe(p.last_line_read());
    // glslangValidator always output the file name, for reasons
    if (line == jute::view::unsafe(file)) continue;
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
