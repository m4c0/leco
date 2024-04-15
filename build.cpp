#define MINIRENT_IMPLEMENTATION
#include "../minirent/minirent.h"
#define MTIME_IMPLEMENTATION
#include "../mtime/mtime.h"
#define MKDIR_IMPLEMENTATION
#include "mkdir.h"
#define SIM_IMPLEMENTATION
#include "sim.hpp"

int compile(const char *in) {
  sim_sbt out{32};
  sim_sb_printf(&out, "out/%s.o", in);

  if (mtime_of(in) < mtime_of(out.buffer))
    return 0;

  fprintf(stderr, "compiling %s\n", in);

  sim_sbt buf{1024};
  sim_sb_printf(&buf, "clang++ -std=c++20 -g -c %s -o %s", in, out.buffer);
  return system(buf.buffer);
}

static int error() {
  fprintf(stderr, "Could not list current directory: %s\n", strerror(errno));
  return 1;
}

int main(int argc, char **argv) {
  mkdirs("out");

  // should we make it work if running in a different dir?
  DIR *dir = opendir(".");
  if (dir == nullptr)
    return error();

  sim_sbt link{10240};
  sim_sb_copy(&link, "clang++ -o leco.exe ");

  dirent *ent{};
  while ((ent = readdir(dir))) {
    if (0 == strcmp("build.cpp", ent->d_name))
      continue;

    auto ext = sim_path_extension(ent->d_name);
    if (ext == nullptr)
      continue;

    if (0 != strcmp(".cpp", ext))
      continue;

    if (0 != compile(ent->d_name))
      return 1;

    sim_sb_printf(&link, " out/%s.o", ent->d_name);
  }
  closedir(dir);

  fprintf(stderr, "linking leco.exe\n");
  return system(link.buffer);
}
