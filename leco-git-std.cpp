#pragma leco tool

#include <stdio.h>

import gopt;
import mtime;
import sim;
import sys;

static void usage() {
  sys::die(R"(Usage: leco.exe git-std [-i <name>]

Clones a given repo, sets up e-mail and git pre-commit hooks.

This is a very selfish tool. I use it to create a standardised repo in all
machines I develop with.

Be aware this tool will also apply configs for user.email and user.name

Where: 
        -i: name of the repo - assumes a GitHub repo like m4c0/<name> and
            defaults to current dir (skipping the clone phase)
)");
}

static void clone(const sim::sb & name) {
  sys::runf("git clone git@github.com:m4c0/%s.git %s", name.path_filename(), *name);
}

static void set_email(const sim::sb & name) {
  sys::runf("git -C %s config user.name 'Eduardo Costa'", *name);
  sys::runf("git -C %s config user.email 'm4c0@users.noreply.github.com'", *name);
}

static void setup_precommit(const sim::sb & name) {
  auto f = sys::fopen(*name, "w");
  fprintf(f, R"(#!/bin/sh

../leco/leco.exe format -n
)");
  fclose(f);
}

int main(int argc, char **argv) try {
  sim::sb name = "."_real;
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
      case 'i': name = sim::path_real(val); break;
      default: usage();
    }
  });
  if (opts.argc) usage();

  if (mtime::of(*name) == 0) clone(name);
  set_email(name);

  name = name / ".git" / "hooks" / "pre-commit";
  if (mtime::of(*name) > 0) {
    fprintf(stderr, "pre-commit hook exists... bailing out...\n");
  } else {
    setup_precommit(name);
  }

#ifndef _WIN32
  sys::runf("chmod a+x %s", *name);
#endif
} catch (...) {
  return 1;
}
