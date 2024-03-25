#pragma once
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct sim_sb {
  char *buffer;
  unsigned size;
  unsigned len;
} sim_sb;
void sim_sb_new(sim_sb *sb, unsigned size);
void sim_sb_delete(sim_sb *sb);
void sim_sb_copy(sim_sb *dst, const char *src);
void sim_sb_concat(sim_sb *dst, const char *src);

__attribute__((format(printf, 2, 3))) void sim_sb_printf(sim_sb *dst,
                                                         const char *src, ...);

void sim_sb_path_append(sim_sb *dst, const char *part);
void sim_sb_path_copy_append(sim_sb *dst, const char *path, const char *part);
void sim_sb_path_parent(sim_sb *dst);
void sim_sb_path_copy_parent(sim_sb *dst, const char *path);
void sim_sb_path_set_extension(sim_sb *dst, const char *ext);

#ifdef __cplusplus
}
struct sim_sbt : sim_sb {
  sim_sbt(unsigned sz) { sim_sb_new(this, sz); }
  ~sim_sbt() { sim_sb_delete(this); }

  sim_sbt(const sim_sbt &) = delete;
  sim_sbt(sim_sbt &&) = delete;
  sim_sbt &operator=(const sim_sbt &) = delete;
  sim_sbt &operator=(sim_sbt &&) = delete;
};
#endif

// TODO: add "SIM_ASSERT"
#define SIM_MALLOC malloc
#define SIM_FREE free

void sim_sb_new(sim_sb *sb, unsigned size) {
  assert(sb && "invalid string buffer");
  assert(size > 0 && "invalid buffer size");

  // One extra for a mandatory cstr terminator
  sb->buffer = (char *)SIM_MALLOC(size + 1);
  assert(sb->buffer && "no memory left");

  memset(sb->buffer, 0, size + 1);
  sb->size = size;
  sb->len = 0;
}
void sim_sb_delete(sim_sb *sb) {
  assert(sb->buffer && "uninitialised buffer");

  SIM_FREE(sb->buffer);
  sb->buffer = NULL;
  sb->len = 0;
  sb->size = 0;
}

void sim_sb_copy(sim_sb *dst, const char *src) {
  assert(dst->buffer && "uninitialised buffer");
  assert(src && "invalid source");

  strncpy(dst->buffer, src, dst->size);
  dst->len = strlen(dst->buffer);
}
void sim_sb_concat(sim_sb *dst, const char *src) {
  assert(dst->buffer && "uninitialised buffer");
  assert(src && "invalid source");

  strncpy(dst->buffer + dst->len, src, dst->size - dst->len);
  dst->len = strlen(dst->buffer);
}

__attribute__((format(printf, 2, 3))) void
sim_sb_printf(sim_sb *dst, const char *src, ...) {
  assert(dst->buffer && "uninitialised buffer");

  va_list va;
  va_start(va, src);
  vsnprintf(dst->buffer + dst->len, dst->size - dst->len, src, va);
  va_end(va);
  dst->len = strlen(dst->buffer);
}

void sim_sb_path_append(sim_sb *dst, const char *part) {
  assert(dst->buffer && "uninitialised buffer");

  sim_sb_concat(dst, "/"); // TODO: flip on windows?
  sim_sb_concat(dst, part);
}
void sim_sb_path_copy_append(sim_sb *dst, const char *path, const char *part) {
  assert(dst->buffer && "uninitialised buffer");
  assert(path && "invalid source path");

  sim_sb_copy(dst, path);
  sim_sb_path_append(dst, part);
}

void sim_sb_path_parent(sim_sb *dst) {
  assert(dst->buffer && "uninitialised buffer");

  char *p = strrchr(dst->buffer, '/');
  if (p == NULL) {
    sim_sb_copy(dst, ".");
    return;
  }
  if (p == dst->buffer) {
    sim_sb_copy(dst, "/");
    return;
  }
  *p = 0;
  dst->len = p - dst->buffer;
}
void sim_sb_path_copy_parent(sim_sb *dst, const char *path) {
  assert(dst->buffer && "uninitialised buffer");
  assert(path && "invalid source path");

  sim_sb_copy(dst, path);
  sim_sb_path_parent(dst);
}

//           /tmp/test ===> /tmp/test.uga
//                   / ===> /
//                 tmp ===> tmp.uga
//       /test/asd.wer ===> /test/asd.uga
//               as.we ===> as.uga
//                     ===>
//    /tmp.ggg/asd.qwe ===> /tmp.ggg/asd.uga
//        /tmp.ggg/qwe ===> /tmp.ggg/qwe.uga
//       /tmp/tmp.xxx/ ===> /tmp/tmp.xxx/
void sim_sb_path_set_extension(sim_sb *dst, const char *ext) {
  assert(dst->buffer && "uninitialised buffer");
  assert(ext && "invalid extension");

  if (dst->len == 0)
    return;

  char *p = strrchr(dst->buffer, '/');
  char *e = strrchr(dst->buffer, '.');
  // Path ends with '/'
  if (p == dst->buffer + dst->len - 1)
    return;
  // No extension or extension isn't in stem
  if (!e || (p && e && p > e)) {
    sim_sb_concat(dst, ".");
    sim_sb_concat(dst, ext);
    return;
  }
  e[1] = 0;
  dst->len = e - dst->buffer + 1;
  sim_sb_concat(dst, ext);
}
