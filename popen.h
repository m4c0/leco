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
#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN
#include <windows.h>

static int proc__create_process(char *cmd_line, HANDLE out, HANDLE err) {
  PROCESS_INFORMATION pi{};
  STARTUPINFO si{};
  si.cb = sizeof(STARTUPINFO);
  si.hStdError = err;
  si.hStdOutput = out;
  si.dwFlags = STARTF_USESTDHANDLES;

  if (!CreateProcess(NULL, cmd_line, NULL, NULL, true, 0, NULL, NULL, &si, &pi))
    return 1;

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  CloseHandle(out);
  CloseHandle(err);
  return 0;
}
static int proc__create_pipes(HANDLE *outs, HANDLE *errs) {
  SECURITY_ATTRIBUTES attr{};
  attr.nLength = sizeof(SECURITY_ATTRIBUTES);
  attr.bInheritHandle = TRUE;
  attr.lpSecurityDescriptor = NULL;

  if (!CreatePipe(outs, outs + 1, &attr, 0))
    return 1;
  if (!SetHandleInformation(outs[0], HANDLE_FLAG_INHERIT, 0))
    return 1;

  if (!CreatePipe(errs, errs + 1, &attr, 0))
    return 1;
  if (!SetHandleInformation(errs[0], HANDLE_FLAG_INHERIT, 0))
    return 1;

  return 0;
}
int proc_open(char *const *cmd_line, FILE **out, FILE **err) {
  HANDLE outs[2];
  HANDLE errs[2];
  if (0 != proc__create_pipes(outs, errs))
    return 1;
  if (0 != proc__create_process(cmd_line, outs[1], errs[1]))
    return 1;

  return 0;
}

#else // !_WIN32
#include <unistd.h>

int proc_open(char *const *cmd_line, FILE **out, FILE **err) {
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
}
#endif // !_WIN32
#endif // POPEN_IMPLEMENTATION
