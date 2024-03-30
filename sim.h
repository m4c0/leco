#ifndef SIM_H
#define SIM_H

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
void sim_sb_path_copy_sb_stem(sim_sb *dst, const sim_sb *src);

void sim_sb_path_copy_real(sim_sb *dst, const char *path);

const char *sim_sb_path_extension(const sim_sb *src);
const char *sim_sb_path_filename(const sim_sb *src);

#ifdef _WIN32
#define SIM_PATHSEP '\\'
#define SIM_PATHSEP_S "\\"
#else
#define SIM_PATHSEP '/'
#define SIM_PATHSEP_S "/"
#endif

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
#endif //__cplusplus
#endif // SIM_H

#ifdef SIM_IMPLEMENTATION

// TODO: add "SIM_ASSERT"
#define SIM_MALLOC malloc
#define SIM_FREE free

#ifdef _WIN32
#define SIM_STRNCPY(Dst, Src, Sz) strcpy_s(Dst, Sz, Src)
#else
#define SIM_STRNCPY strncpy
#endif

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

  SIM_STRNCPY(dst->buffer, src, dst->size);
  dst->len = strlen(dst->buffer);
}
void sim_sb_concat(sim_sb *dst, const char *src) {
  assert(dst->buffer && "uninitialised buffer");
  assert(src && "invalid source");

  SIM_STRNCPY(dst->buffer + dst->len, src, dst->size - dst->len);
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

  sim_sb_concat(dst, SIM_PATHSEP_S);
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

  char *p = strrchr(dst->buffer, SIM_PATHSEP);
  if (p == NULL) {
    // TODO: decide how to deal with parent of "."
    sim_sb_copy(dst, ".");
    return;
  }
  if (p == dst->buffer) {
    sim_sb_copy(dst, SIM_PATHSEP_S);
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

const char *sim_sb_path_extension(const sim_sb *src) {
  assert(src->buffer && "uninitialised buffer");

  if (src->len == 0)
    return nullptr;

  char *p = strrchr(src->buffer, SIM_PATHSEP);
  char *e = strrchr(src->buffer, '.');
  // Path ends with SIM_PATHSEP
  if (p == src->buffer + src->len - 1)
    return nullptr;
  // No extension or extension isn't in stem
  if (!e || (p && e && p > e)) {
    return nullptr;
  }
  return e;
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

  char *p = strrchr(dst->buffer, SIM_PATHSEP);
  char *e = strrchr(dst->buffer, '.');
  // Path ends with SIM_PATHSEP
  if (p == dst->buffer + dst->len - 1)
    return;
  // No extension or extension isn't in stem
  if (!e || (p && e && p > e)) {
    sim_sb_concat(dst, ".");
    sim_sb_concat(dst, ext);
    return;
  }
  if (*ext == 0) { // remove extension if ext is empty
    *e = 0;
    dst->len = e - dst->buffer;
    return;
  }
  e[1] = 0;
  dst->len = e - dst->buffer + 1;
  sim_sb_concat(dst, ext);
}

const char *sim_sb_path_filename(const sim_sb *src) {
  assert(src->buffer && "uninitialised buffer");

  if (src->len == 0)
    return nullptr;

  char *p = strrchr(src->buffer, SIM_PATHSEP);
  if (p == nullptr)
    return ".";
  if (p == src->buffer)
    return SIM_PATHSEP_S;

  // TODO: handle slashes at the end
  return ++p;
}

void sim_sb_path_copy_sb_stem(sim_sb *dst, const sim_sb *src) {
  sim_sb_copy(dst, sim_sb_path_filename(src));
  sim_sb_path_set_extension(dst, "");
}

void sim_sb_path_copy_real(sim_sb *dst, const char *path) {
  assert(dst->buffer && "uninitialised buffer");
#ifdef _WIN32
  _fullpath(dst->buffer, path, dst->size);
#else
  assert(dst->size >= PATH_MAX &&
         "destination buffer should have PATH_MAX in size");
  realpath(path, dst->buffer);
#endif
  dst->len = strlen(dst->buffer);
}
#endif // SIM_IMPLEMENTATION
