#pragma leco tool

#include "sim.hpp"
#include "targets.hpp"

import sys;

void usage() {
  sys::die(R"(
Uploads an iOS application. This assumes everything else was generated, signed,
exported, etc.

Must be run from the root of the source repository.

It only support iPhoneOS exports. No support for other Apple OSs like WatchOS,
etc.

Usage: ../leco/leco.exe ipa-upload -i <input-dag>

)");
}

int main(int argc, char ** argv) try {
  sys::run("xcrun altool");

  return 0;
} catch (...) {
  return 1;
}
