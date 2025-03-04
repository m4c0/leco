#pragma leco tool
#include <stdio.h>

import gopt;
import sim;
import sys;

static void usage() { sys::die("invalid usage"); }

static void process_file(const char * dag, const char * file) {
  auto path = sim::path_parent(dag);
  auto parent = sim::path_parent(*path);
  path /= sim::path_filename(file);
  path += ".hpp";

  // TODO: check if newer
  sys::log("generating", *path);

  sim::sb id { sim::path_filename(file) };
  for (auto i = 0; i < id.len; i++) {
    auto & c = id[i];
    if (c == '.') c = '_';
  }

  parent.path_parent();
  auto pname = sim::path_filename(*parent);

  auto f = sys::fopen(*path, "wb");
  fprintf(f, "#pragma once\n");
  fprintf(f, "// Size includes null-terminator. Equivalent of this line...\n");
  fprintf(f, "// static constexpr const char %s_%s[] = \"...\";\n", pname, *id);
  fprintf(f, "// ...with size included for information purposes.\n");

  auto in = sys::fopen(file, "rb");
  fseek(in, 0, SEEK_END);
  int size = ftell(in);
  fseek(in, 0, SEEK_SET);

  fprintf(f, "static constexpr const char %s_%s[%d] =", pname, *id, size + 1);

  while (!feof(in)) {
    fprintf(f, "\n  \"");
    char buf[32];
    int n = fread(buf, 1, 32, in);
    for (auto i = 0; i < n; i++) {
      char c = buf[i];
      // if (c >= 32 && c < 127 && c != '"') {
      //   fputc(c, f);
      // } else {
        fprintf(f, "\\%03o", (unsigned)c & 0xFF);
      // }
    }
    fprintf(f, "\"");
  }
  fclose(in);

  fprintf(f, ";\n");
  fclose(f);
}

int main(int argc, char ** argv) {
  const char * target = nullptr;
  auto opts = gopt_parse(argc, argv, "t:", [&](auto ch, auto var) {
    switch (ch) {
      case 't': target = var; break;
      default: usage();
    }
  });
  if (!target || opts.argc) usage();

  sys::for_each_dag(target, true, [](const char * dag, auto id, auto file) {
    if (id != 'embd') return;
    process_file(dag, file);
  });
}
