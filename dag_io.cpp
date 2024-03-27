#include "dag.hpp"
#include <fstream>

static void persist(std::ostream &o, const llvm::StringSet<> &items) {
  o << items.size() << "\n";
  for (const auto &s : items) {
    o << s.first().str() << "\n";
  }
}
static void persist(std::ostream &o, const dag::node *n) {
  o << "LECO 1\n"; // fourcc-ish+version
  o << static_cast<int>(n->root_type()) << "\n";
  persist(o, n->executables());
  persist(o, n->frameworks());
  persist(o, n->include_dirs());
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

static bool read(std::istream &f, dag::node *n, llvm::StringSet<> *set) {
  int size{};
  f >> size;
  char c = f.get();
  if (c != '\n') {
    dag::errlog(n, "invalid char after dag list size");
    return false;
  }

  for (auto i = 0; i < size; i++) {
    std::string line{};
    if (!std::getline(f, line)) {
      dag::errlog(n, "error in dag entry");
      return false;
    }
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
  if (magic != "LECO") {
    dag::errlog(this, "invalid dag cache");
    return false;
  }

  int ver{};
  f >> ver;
  if (ver != 1) {
    dag::errlog(this, "invalid dag cache version");
    return false;
  }

  // TODO: validate CRC or something

  int r;
  f >> r;
  m_root = static_cast<root_t>(r);

  return read(f, this, &m_executables) && read(f, this, &m_frameworks) &&
         read(f, this, &m_include_dirs) && read(f, this, &m_libraries) &&
         read(f, this, &m_library_dirs) && read(f, this, &m_mod_deps) &&
         read(f, this, &m_mod_impls) && read(f, this, &m_resources) &&
         read(f, this, &m_shaders);
}
