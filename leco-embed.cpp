#pragma leco tool
#include <stdio.h>

import sys;

static void process_file(const char * dag, const char * file) {
  auto path = sim::path_parent(dag);
  auto parent = sim::path_parent(*path);
  path /= sim::path_filename(file);
  path += ".hpp";

  if (mtime::of(file) < mtime::of(*path)) return;

  sys::log("generating", *path);

  sim::sb id { sim::path_filename(file) };
  for (auto i = 0; i < id.len; i++) {
    auto & c = id[i];
    if (c == '.') c = '_';
  }

  parent.path_parent();
  auto pname = sim::path_filename(*parent);

  sys::file f { *path, "wb" };
  fprintf(f, "#pragma once\n");
  fprintf(f, "// Size includes null-terminator. Equivalent of this line...\n");
  fprintf(f, "// static constexpr const char %s_%s[] = \"...\";\n", pname, *id);
  fprintf(f, "// ...with size included for information purposes.\n");

  sys::file in { file, "rb" };
  fseek(in, 0, SEEK_END);
  int size = ftell(in);
  fseek(in, 0, SEEK_SET);

  fprintf(f, "static constexpr const char %s_%s[%d] =", pname, *id, size + 1);

  while (!feof(in)) {
    fprintf(f, "\n  \"");
    char buf[32];
    int n = fread(buf, 1, 32, in);
    for (auto i = 0; i < n; i++) fprintf(f, "\\%03o", (unsigned)buf[i] & 0xFF);
    fprintf(f, "\"");
  }

  fprintf(f, ";\n");
}

int main() {
  sys::for_each_tag_in_dags('embd', true, &process_file);
}
