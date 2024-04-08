#define MTIME_IMPLEMENTATION
#include "../mtime/mtime.h"
#define MKDIR_IMPLEMENTATION
#include "mkdir.h"
#define SIM_IMPLEMENTATION
#include "sim.hpp"

static constexpr const char *files[]{
    "actool",  "bouncer", "cl",     "cleaner",    "compile",
    "context", "dag",     "dag_io", "dag_plugin", "droid_path",
    "evoker",  "impls",   "link",   "plist",      "target_defs",
};

bool compile(const char *stem) {
  sim_sbt in{32};
  sim_sbt out{32};
  sim_sb_printf(&in, "%s.cpp", stem);
  sim_sb_printf(&out, "out/%s.o", stem);

  if (mtime_of(in.buffer) < mtime_of(out.buffer))
    return true;

  fprintf(stderr, "compiling %s\n", stem);

  sim_sbt buf{1024};
  sim_sb_printf(&buf, "clang++ -std=c++20 -g -c %s -o %s", in.buffer,
                out.buffer);
  return 0 == system(buf.buffer);
}
bool link(const char *outf) {
  sim_sbt buf{10240};
  sim_sb_copy(&buf, "clang++ ");
  for (auto f : files) {
    sim_sb_printf(&buf, " out/%s.o ", f);
  }
  sim_sb_concat(&buf, outf);
  return 0 == system(buf.buffer);
}

int main(int argc, char **argv) {
  mkdirs("out");
  for (auto f : files) {
    if (!compile(f))
      return 1;
  }
  if (!compile("leco"))
    return 1;
  if (!link("out/leco.o -o leco.exe"))
    return 1;
  return 0;
}
