#include <string.h>

#if _WIN32
#include <process.h>
#define execvp _execvp
#define strdup _strdup
#define SEP "\\"
#else
#include <unistd.h>
#define SEP "/"
#endif

#include "targets.hpp"

#define CMD ".." SEP "leco" SEP "out" SEP HOST_TARGET SEP "leco-meta.exe"

int main(int argc, char ** argv) {
  argv[0] = strdup(CMD);
  return execvp(CMD, argv);
}
