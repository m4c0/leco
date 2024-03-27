#pragma once
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"

namespace dag {
enum class root_t {
  none,
  main_mod,
  dll,
  tool,
  app,
};

class node {
  llvm::SmallString<256> m_source;
  llvm::SmallString<256> m_target;
  llvm::SmallString<256> m_dag;
  llvm::SmallString<256> m_module_name;
  llvm::SmallString<256> m_module_pcm;
  llvm::StringSet<> m_executables{};
  llvm::StringSet<> m_frameworks{};
  llvm::StringSet<> m_include_dirs{};
  llvm::StringSet<> m_libraries{};
  llvm::StringSet<> m_library_dirs{};
  llvm::StringSet<> m_mod_deps{};
  llvm::StringSet<> m_mod_impls{};
  llvm::StringSet<> m_resources{};
  llvm::StringSet<> m_shaders{};
  bool m_compiled{};
  root_t m_root{};
  bool m_recursed{};

public:
  explicit node(const char *n);

  bool read_from_cache_file();
  void write_to_cache_file() const;

  [[nodiscard]] bool add_executable(llvm::StringRef e);
  [[nodiscard]] bool add_include_dir(llvm::StringRef dir);
  void add_framework(llvm::StringRef fw) { m_frameworks.insert(fw); }
  void add_library(llvm::StringRef lib) { m_libraries.insert(lib); }
  [[nodiscard]] bool add_library_dir(llvm::StringRef dir);
  [[nodiscard]] bool add_mod_dep(llvm::StringRef mod_name);
  [[nodiscard]] bool add_mod_impl(llvm::StringRef mod_impl);
  [[nodiscard]] bool add_resource(llvm::StringRef res);
  [[nodiscard]] bool add_shader(llvm::StringRef shd);

  void set_app() { m_root = root_t::app; }
  void set_compiled() { m_compiled = true; }
  void set_dll() { m_root = root_t::dll; }
  void set_main_mod() { m_root = root_t::main_mod; }
  void set_recursed() { m_recursed = true; }
  void set_tool() { m_root = root_t::tool; }

  [[nodiscard]] constexpr const auto &executables() const noexcept {
    return m_executables;
  }
  [[nodiscard]] constexpr const auto &frameworks() const noexcept {
    return m_frameworks;
  }
  [[nodiscard]] constexpr const auto &include_dirs() const noexcept {
    return m_include_dirs;
  }
  [[nodiscard]] constexpr const auto &libraries() const noexcept {
    return m_libraries;
  }
  [[nodiscard]] constexpr const auto &library_dirs() const noexcept {
    return m_library_dirs;
  }
  [[nodiscard]] constexpr const auto &mod_deps() const noexcept {
    return m_mod_deps;
  }
  [[nodiscard]] constexpr const auto &mod_impls() const noexcept {
    return m_mod_impls;
  }
  [[nodiscard]] constexpr const auto &resources() const noexcept {
    return m_resources;
  }
  [[nodiscard]] constexpr const auto &shaders() const noexcept {
    return m_shaders;
  }

  [[nodiscard]] constexpr auto root_type() const noexcept { return m_root; }

  [[nodiscard]] constexpr bool app() const noexcept {
    return m_root == root_t::app;
  }
  [[nodiscard]] constexpr bool compiled() const noexcept { return m_compiled; }
  [[nodiscard]] constexpr bool dll() const noexcept {
    return m_root == root_t::dll;
  }
  [[nodiscard]] constexpr bool recursed() const noexcept { return m_recursed; }
  [[nodiscard]] constexpr bool root() const noexcept {
    return m_root != root_t::none;
  }
  [[nodiscard]] constexpr bool tool() const noexcept {
    return m_root == root_t::tool;
  }

  [[nodiscard]] constexpr llvm::StringRef source() const noexcept {
    return m_source;
  }
  [[nodiscard]] constexpr llvm::StringRef target() const noexcept {
    return m_target;
  }
  [[nodiscard]] constexpr llvm::StringRef dag() const noexcept { return m_dag; }
  [[nodiscard]] constexpr llvm::StringRef module_name() const noexcept {
    return m_module_name;
  }
  [[nodiscard]] constexpr llvm::StringRef module_pcm() const noexcept {
    return m_module_pcm;
  }
};
[[nodiscard]] bool execute(node *n);

void xlog(const node *n, const char *msg);
void errlog(const node *n, const char *msg);

node *get_node(const char *source);
node *process(const char *path);
void clear_cache();

void visit(const node *n, bool impls, auto &&fn) {
  llvm::StringSet<> visited{};

  const auto rec = [&](auto rec, auto *n) {
    if (visited.contains(n->source()))
      return;

    for (auto &d : n->mod_deps()) {
      rec(rec, get_node(d.first().str().c_str()));
    }

    fn(n);
    visited.insert(n->source());

    if (impls)
      for (auto &d : n->mod_impls()) {
        rec(rec, get_node(d.first().str().c_str()));
      }
  };
  rec(rec, n);
}
uint64_t visit_dirty(const node *n, llvm::function_ref<void(const node *)> fn);
} // namespace dag
