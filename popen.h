#ifndef POPEN_H
#define POPEN_H
#ifdef __cplusplus
extern "C" {
#endif

int proc_open(const char *cmd_line, FILE **out, FILE **err);

#ifdef __cplusplus
}
#endif
#endif // POPEN_H

#ifdef POPEN_IMPLEMENTATION
int proc_open(const char *cmd_line, FILE **out, FILE **err) { return 0; }
#endif
