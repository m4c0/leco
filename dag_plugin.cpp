#include "dag.hpp"

#include "cl.hpp"
#include "context.hpp"
#include "dag2.hpp"
#include "die.hpp"

#include <string.h>

void dag::node::read_from_cache_file() {
  dag_read(dag(), [this](auto id, auto file) {
    switch (id) {
    case 'tool':
      set_tool();
      break;
    case 'tapp':
      set_app();
      break;
    case 'tdll':
      set_dll();
      break;
    case 'tmmd':
      set_main_mod();
      break;
    case 'bdep':
      add_build_dep(file);
      break;
    case 'head':
      add_header(file);
      break;
    case 'impl':
      add_mod_impl(file);
      break;
    case 'mdep':
      add_mod_dep(file);
      break;
    }
  });
}
