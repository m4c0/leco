#ifndef POPEN_H
#define POPEN_H
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int proc_open(char *const *cmd_line, FILE **out, FILE **err);

#ifdef __cplusplus
}
#endif
#endif // POPEN_H

#ifdef POPEN_IMPLEMENTATION
#ifndef _WIN32
#include <unistd.h>
#endif

int proc_open(char *const *cmd_line, FILE **out, FILE **err) {
#ifdef _WIN32
#else
  int pout[2];
  if (0 != pipe(pout))
    return -1;

  int perr[2];
  if (0 != pipe(perr))
    return -1;

  int pid = fork();
  if (pid < 0) {
    return -1;
  }
  if (pid == 0) {
    close(0);
    close(pout[0]);
    close(perr[0]);
    dup2(pout[1], 1);
    dup2(perr[1], 2);
    execv(cmd_line[0], cmd_line + 1);
    return -1;
  }

  close(pout[1]);
  close(perr[1]);

  *out = fdopen(pout[0], "r");
  *err = fdopen(perr[0], "r");
  return 0;
#endif
}
#endif
