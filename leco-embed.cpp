#pragma leco tool

import gopt;
import sim;
import sys;

static void usage() { sys::die("invalid usage"); }

static void process_file(const char * dag, const char * file) {
  auto path = sim::path_parent(dag);
  path /= *sim::path_stem(file);
  path.path_extension("hpp");

  sys::log("generating", *path);
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
