#pragma leco tool
#include <stdio.h>

int main(int argc, char **argv) {
  fprintf(stderr, R"(
Usage: ../leco/leco.exe <tool> <tool-options>

Porcelain tools:

        cleaner: cleans output folders
        driver: drives the LECO workflow (default tool if unspecified)
        format: formats code using clang-format
        git-status: list status of all folders in the git workspace
        git-update: recursively updates known dependencies
        help: this tool
        ipa-upload: validates/upload iOS binaries

Plumbing tools:

        bundler: bundles applications
        clang: runs clang with opiniated defaults
        codesign: runs codesign on Apple platforms
        dagger: generates dependency files based on parsed code
        deplist: list dependent c++ modules
        exs: copies dependent executables to app bundles
        ipa: generates and signs IPAs
        link: links an executable
        recurse: compiles C++ code recursively
        rsrc: copies resource files to their app's resource folder
        sawblade: runs dagger recursively
        shaders: compiles GLSL into SPIRV
        sysroot: calculates the sysroot of the environment
        wasm-js: almost a webpacker
        xcassets: generates xcassets for IPAs

)");
}
