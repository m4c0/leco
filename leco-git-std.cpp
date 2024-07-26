#pragma leco tool
#define SIM_IMPLEMENTATION

#include "fopen.hpp"
#include "sim.hpp"

#include <stdio.h>

import gopt;
import mtime;
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

static void clone(const sim_sb *name) {
  auto fn = sim_sb_path_filename(name);

  sim_sbt cmd{10240};
  sim_sb_printf(&cmd, "git clone git@github.com:m4c0/%s.git %s", fn,
                name->buffer);
  sys::run(cmd.buffer);
}

static void set_email(const sim_sb *name) {
  sim_sbt cmd{10240};
  sim_sb_printf(&cmd, "git -C %s config user.name 'Eduardo Costa'",
                name->buffer);
  sys::run(cmd.buffer);

  sim_sb_copy(&cmd, "");
  sim_sb_printf(&cmd,
                "git -C %s config user.email 'm4c0@users.noreply.github.com'",
                name->buffer);
  sys::run(cmd.buffer);
}

static void setup_precommit(const sim_sb *name) {
  f::open f{name->buffer, "w"};
  fprintf(*f, R"(#!/bin/sh

../leco/leco.exe format -n
)");
}

int main(int argc, char **argv) try {
  sim_sbt name{};
  auto opts = gopt_parse(argc, argv, "i:", [&](auto ch, auto val) {
    switch (ch) {
    case 'i':
      sim_sb_path_copy_real(&name, val);
      break;
    default:
      usage();
    }
  });

  if (opts.argc)
    usage();

  if (name.len == 0)
    sim_sb_path_copy_real(&name, ".");

  if (mtime::of(name.buffer) == 0)
    clone(&name);

  set_email(&name);

  sim_sb_path_append(&name, ".git");
  sim_sb_path_append(&name, "hooks");
  sim_sb_path_append(&name, "pre-commit");
  if (mtime::of(name.buffer) > 0) {
    fprintf(stderr, "pre-commit hook exists... bailing out...\n");
  } else {
    setup_precommit(&name);
  }

#ifndef _WIN32
  sim_sbt chmod{10240};
  sim_sb_printf(&chmod, "chmod a+x %s", name.buffer);
  sys::run(chmod.buffer);
#endif
} catch (...) {
  return 1;
}
