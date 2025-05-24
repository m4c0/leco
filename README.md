# leco

Lean Ecossystem for C++ and Objective-C.

This is a build system which uses C++ code and some conventions to build an
entire world of modules.

Intended usage:
1. Create a "workspace" directory
2. Clone LECO
3. Clone these repos:
    * [gopt][gopt] - GNU-like command line parser
    * [mtime][mtime] - "stat" wrapper
    * [popen][popen] - "popen" wrapper
    * [pprent][pprent] - "[minirent][minirent]" wrapper (i.e. "dirent" wrapper)
4. Clone/create intended C++ repos in that workspace
5. Build LECO
6. Use LECO to build everything else

[gopt]: https://github.com/m4c0/gopt.git
[minirent]: https://github.com/tsoding/minirent.git
[mtime]: https://github.com/m4c0/mtime.git
[popen]: https://github.com/m4c0/popen.git
[pprent]: https://github.com/m4c0/pprent.git

Some rules:
* Each C++ module (and its parts/impls) should reside in a directory named
  after that module
* Each C++ code should use pragmas to represent non-module dependencies (flags,
  resources, system libraries, etc)

## Building LECO

Roughly:

```sh
clang++ -std=c++20 -o build.exe build.cpp
build.exe
```

`build.exe` only requires a clang++ installed to be built. It does not require
LECO to be installed afterwards.

If this documentation is outdated and any modules required aren't informed
here, you will face an error with message `module not found` - pointing to
the failed `import` line. You can use the module name to find a repository with
the same name in [my personal GitHub page][repos].

On Mac, it requires LLVM from Home Brew. At the time of writing, Mac's clang
only support the old TS module system.

Also on Mac, you need the iOS SDK if you want to use LECO to build iOS apps.

[repos]: https://github.com/m4c0?tab=repositories

## Running LECO

For running it on Windows and linux, `clang++` must be at least v17 and
available on PATH. On MacOS, it must be installed via HomeBrew.

LECO can be run from any LECO-based repo in the workspace (even on LECO repo
itself). A command like this is enough to build a project:

```sh
../leco/leco.exe
```

On Windows CMD, you need to use the reverse slash. On non-Windows, an `.exe`
extension is added to all executables created by LECO. This simplifies any
cross-platform automations (like LECO itself). It might be weird for Linux or
Mac-exclusive users, but it definitely helps for anyone working on different
platforms.

The main `leco.exe` tool is a facade for other smaller tools - similar to how
`git` supports multiple subcommands. The default action is to build the code in
the current directory. It's equivalent to call:

```sh
../leco/leco.exe driver
```

Other subcommands are documented via the `help` tool:

```sh
../leco/leco.exe help
```

