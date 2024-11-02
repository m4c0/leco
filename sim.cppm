module;
#include "sim.h"

#ifdef __linux__
#include <linux/limits.h>
#elif _WIN32
#include <stdlib.h>
#define PATH_MAX _MAX_PATH
#else
#include <limits.h>
#endif

export module sim;

export namespace sim {
  struct sb : sim_sb {
    sb() { sim_sb_new(this, PATH_MAX); }
    sb(unsigned sz) { sim_sb_new(this, sz); }
    ~sb() { if (buffer) sim_sb_delete(this); }

    sb(const sb & o) : sb() { sim_sb_copy(this, o.buffer); }
    sb(sb && o) : sim_sb { o } { static_cast<sim_sb &>(o) = {}; }
    sb & operator=(const sb & o) {
      sim_sb_copy(this, o.buffer);
      return *this;
    }
    sb & operator=(sb && o) {
      if (buffer) sim_sb_delete(this);
      static_cast<sim_sb &>(*this) = o;
      static_cast<sim_sb &>(o) = {};
      return *this;
    }

    explicit sb(const char * s) : sb {} { sim_sb_copy(this, s); }

    char * operator*() { return buffer; }
    const char * operator*() const { return buffer; }

    sb & operator+=(const char * s) {
      sim_sb_concat(this, s);
      return *this;
    }
    sb & operator/=(const char * s) {
      sim_sb_path_append(this, s);
      return *this;
    }

    sb & printf(const char * fmt, auto ... args) {
      sim_sb_printf(this, fmt, args...);
      return *this;
    }

    const char * path_filename() const { return sim_sb_path_filename(this); }

    const char * path_extension() const { return sim_sb_path_extension(this); }
    void path_extension(const char * ext) { sim_sb_path_set_extension(this, ext); }

    void path_parent() { sim_sb_path_parent(this); }
  };

  sb path_extension(const char * path) {
    return sim::sb { sim_path_extension(path) };
  }
  sb path_real(const char * path) {
    sim::sb res {};
    sim_sb_path_copy_real(&res, path);
    return res;
  }
  sb path_stem(const char * path) {
    sb res {};
    sim_sb_path_copy_stem(&res, path);
    return res;
  }
  sb path_parent(const char * path) {
    sb res {};
    sim_sb_path_copy_parent(&res, path);
    return res;
  }
  sb printf(const char * fmt, auto ... args) {
    sb res {};
    sim_sb_printf(&res, fmt, args...);
    return res;
  }

  sb operator+(const sb & a, const char * b) {
    sb res = a;
    sim_sb_concat(&res, b);
    return res;
  }
  sb operator/(const sb & a, const char * b) {
    sb res = a;
    sim_sb_path_append(&res, b);
    return res;
  }

  const char * path_filename(const char * path) { return sim_path_filename(path); }
} // namespace sim

export sim::sb operator""_real(const char * str, decltype(sizeof(void *)) sz) { return sim::path_real(str); }
