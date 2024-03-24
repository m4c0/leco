#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
struct sim_sb {
  char *buffer;
  unsigned size;
  unsigned len;
};
#ifdef __cplusplus
}
#endif

#define SIM_MALLOC malloc
#define SIM_FREE free

bool sim_sb_new(sim_sb *sb, unsigned size) {
  sb->buffer = (char *)SIM_MALLOC(size + 1);
  memset(sb->buffer, 0, size + 1);
  sb->size = size;
  sb->len = 0;
  return sb->buffer != NULL;
}
void sim_sb_delete(sim_sb *sb) {
  SIM_FREE(sb->buffer);
  *sb = {0};
}

void sim_sb_copy(sim_sb *dst, const char *src) {
  strncpy(dst->buffer, src, dst->size);
  dst->len = strlen(dst->buffer);
}
void sim_sb_concat(sim_sb *dst, const char *src) {
  strncpy(dst->buffer + dst->len, src, dst->size - dst->len);
  dst->len = strlen(dst->buffer);
}

__attribute__((format(printf, 2, 3))) void
sim_sb_printf(sim_sb *dst, const char *src, ...) {
  va_list va;
  va_start(va, src);
  vsnprintf(dst->buffer + dst->len, dst->size - dst->len, src, va);
  va_end(va);
}
