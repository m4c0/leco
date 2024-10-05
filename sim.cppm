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
    ~sb() { sim_sb_delete(this); }

    sb(const sb & o) : sb() { sim_sb_copy(this, o.buffer); }
    sb(sb && o) : sim_sb { o } { static_cast<sim_sb &>(o) = {}; }
    sb & operator=(const sb &) = delete;
    sb & operator=(sb &&) = delete;

    explicit sb(const char * s) : sb {} { sim_sb_copy(this, s); }

    const char * operator*() { return buffer; }

    sb & operator+=(const char * s) {
      sim_sb_concat(this, s);
      return *this;
    }
    sb & operator/=(const char * s) {
      sim_sb_path_append(this, s);
      return *this;
    }
  };
} // namespace sim

export sim::sb operator""_real(const char * str, unsigned long sz) {
  sim::sb res {};
  sim_sb_path_copy_real(&res, str);
  return res;
}
