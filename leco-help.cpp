#pragma leco tool
#include <stdio.h>

int main(int argc, char **argv) {
  fprintf(stderr, R"(
Usage: ../leco/leco.exe <tool> <tool-options>

Existing tools:

        cleaner: cleans output folders
        driver: drives the LECO workflow (default tool if unspecified)
        git-status: list status of all folders in the git workspace
        git-update: recursively updates known dependencies
        help: this tool

Low-level plumbing tools:

        bundler: bundles applications
        clang: runs clang with opiniated defaults
        dagger: generates dependency files based on parsed code
        deplist: list dependent c++ modules
        exs: copies dependent executables to app bundles
        ipa: generates and signs IPAs
        link: links an executable
        recurse: compiles C++ code recursively
        sysroot: calculates the sysroot of the environment
        xcassets: generates xcassets for IPAs

)");
}
