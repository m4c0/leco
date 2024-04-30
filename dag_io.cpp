#include "../mtime/mtime.h"
#include "dag.hpp"
#include "log.hpp"

#include <fstream>

bool dag::node::is_cache_file_fresh() const {
  return mtime_of(dag()) >= mtime_of(source());
}

static void persist(std::ostream &o, const std::set<std::string> &items) {
  o << items.size() << "\n";
  for (const auto &s : items) {
    o << s << "\n";
  }
}
static void persist(std::ostream &o, const dag::node *n) {
  o << "LECO 2\n"; // fourcc-ish+version
  o << static_cast<int>(n->root_type()) << "\n";
  persist(o, n->build_deps());
  persist(o, n->executables());
  persist(o, n->frameworks());
  persist(o, n->headers());
  persist(o, n->libraries());
  persist(o, n->library_dirs());
  persist(o, n->mod_deps());
  persist(o, n->mod_impls());
  persist(o, n->resources());
  persist(o, n->shaders());
}
void dag::node::write_to_cache_file() const {
  // TODO: create a proper file format
  std::ofstream of{dag()};
  persist(of, this);
}

static bool failed(const dag::node *n, const char *msg) {
  elog("failed", n->source());
  elog("", msg);
  return false;
}

static bool read(std::istream &f, dag::node *n, std::set<std::string> *set) {
  int size{};
  f >> size;
  char c = f.get();
  if (c != '\n')
    return failed(n, "invalid char after dag list size");

  for (auto i = 0; i < size; i++) {
    std::string line{};
    if (!std::getline(f, line))
      return failed(n, "error in dag entry");

    set->insert(line);
  }
  return true;
}
bool dag::node::read_from_cache_file() {
  std::ifstream f{dag()};
  if (!f)
    return false;

  std::string magic{};
  f >> magic;
  if (magic != "LECO")
    return failed(this, "invalid dag cache");

  int ver{};
  f >> ver;
  if (ver != 2)
    return failed(this, "invalid dag cache version");

  // TODO: validate CRC or something

  int r;
  f >> r;
  m_root = static_cast<root_t>(r);

  return read(f, this, &m_build_deps) && read(f, this, &m_executables) &&
         read(f, this, &m_frameworks) && read(f, this, &m_headers) &&
         read(f, this, &m_libraries) && read(f, this, &m_library_dirs) &&
         read(f, this, &m_mod_deps) && read(f, this, &m_mod_impls) &&
         read(f, this, &m_resources) && read(f, this, &m_shaders);
}
