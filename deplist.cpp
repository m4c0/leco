#define GOPT_IMPLEMENTATION

#include "../gopt/gopt.h"
#include "die.hpp"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void usage() { die("invalid usage"); }

void read_dag(const char *dag) {
  FILE *f = fopen(dag, "r");
  if (!f)
    die("dag file not found: [%s]\n", dag);

  char buf[10240];
  while (!feof(f) && fgets(buf, sizeof(buf), f) != nullptr) {
    if (strlen(buf) < 5)
      die("invalid line in dag file");

    uint32_t *id = reinterpret_cast<uint32_t *>(buf);
    char *file = reinterpret_cast<char *>(id + 1);
    file[strlen(file) - 1] = 0;

    // process line
  }

  fclose(f);
}

void run(int argc, char **argv) {
  struct gopt opts;
  GOPT(opts, argc, argv, "");

  char *val{};
  char ch;
  while ((ch = gopt_parse(&opts, &val)) != 0) {
    switch (ch) {
    default:
      usage();
    }
  }
  if (opts.argc == 0)
    usage();

  for (auto i = 0; i < opts.argc; i++) {
    read_dag(opts.argv[i]);
  }
}

int main(int argc, char **argv) {
  try {
    run(argc, argv);
    return 0;
  } catch (...) {
    return 1;
  }
}
