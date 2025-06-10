#pragma leco tool
#include <stdio.h>

int main() {
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

        clang: runs clang with opiniated defaults
        codesign: runs codesign on Apple platforms
        dagger: generates dependency files based on parsed code
        deplist: list dependent c++ modules
        driver: drives the build pipeline
        exs: copies dependent executables to app bundles
        ipa: generates and signs IPAs
        ipa-export: exports an IPA archive
        link: links an executable
        meta: used to run other tools
        rc: generates "RC" files for Windows
        rsrc: copies resource files to their app's resource folder
        shaders: compiles GLSL into SPIRV
        sysroot: calculates the sysroot of the environment
        wasm-js: almost a webpacker
        xcassets: generates xcassets for IPAs

)");
}
