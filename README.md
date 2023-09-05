# leco

Lean Ecossystem for C++ and Others.

This is a build system which uses C++ code and some conventions to build an
entire world of modules.

Intended usage:
1. Create a "workspace" directory
2. Clone LECO and intended C++ repos in that workspace
3. Build LECO
4. Use LECO to build everything else

Some rules:
* Each C++ module (and its parts/impls) should reside in a directory named
  after that module
* Each C++ code should use pragmas to represent non-module dependencies (flags,
  resources, system libraries, etc

## Building LECO

Roughly:

```sh
clang++ -std=c+20 -o build.exe build.cppm
build.exe
```

`build.exe` only requires a clang++ installed to be built, but it requires
LLVM/Clang include/libs to build LECO itself.
