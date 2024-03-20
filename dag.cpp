#include "dag.hpp"

#include "../mtime/mtime.h"
#include "cl.hpp"
#include "diags.hpp"
#include "evoker.hpp"
#include "in2out.hpp"
#include "pragma.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/FileSystem.h"
#include <fstream>

using namespace clang;
using namespace llvm;

using time_point = sys::TimePoint<>;

static void real_abs(SmallVectorImpl<char> &buf, StringRef path) {
  sys::fs::real_path(path, buf);
  sys::fs::make_absolute(buf);
}
[[nodiscard]] static bool add_real_abs(StringSet<> &set, StringRef path,
                                       bool is_file = true) {
  SmallString<256> abs{};
  real_abs(abs, path);
  if (is_file && !sys::fs::is_regular_file(abs))
    return false;
  if (!is_file && !sys::fs::is_directory(abs))
    return false;

  set.insert(abs);
  return true;
}

dag::node::node(StringRef n) : m_source{} {
  real_abs(m_source, n);
  in2out(m_source, m_target, "o");
  in2out(m_source, m_dag, "dag");
  in2out(m_source, m_module_pcm, "pcm");
  m_source.c_str();
  m_target.c_str();
  m_module_pcm.c_str();

  auto stem = sys::path::stem(n);
  auto p = stem.find("-");
  if (p != StringRef::npos) {
    auto me = stem.substr(0, p);
    auto part = stem.substr(p + 1);

    sys::path::append(m_module_name, me + ":" + part);
  } else {
    sys::path::append(m_module_name, stem);
  }
  m_module_name.c_str();
}

bool dag::node::add_executable(llvm::StringRef executable) {
  return add_real_abs(m_executables, executable);
}
bool dag::node::add_library_dir(llvm::StringRef dir) {
  return add_real_abs(m_library_dirs, dir, false);
}
bool dag::node::add_mod_dep(llvm::StringRef mod_name) {
  auto dir = sys::path::parent_path(source());

  auto p = mod_name.find(":");
  SmallString<128> dep{};
  if (p != StringRef::npos) {
    auto me = mod_name.substr(0, p);
    auto part = mod_name.substr(p + 1);

    sys::path::append(dep, dir, me + "-" + part + ".cppm");
  } else {
    auto t = mod_name + ".cppm";
    sys::path::append(dep, dir, t);
    if (!sys::fs::is_regular_file(dep)) {
      dep.clear();
      sys::path::append(dep, "..", mod_name, t);
    }
    if (!sys::fs::is_regular_file(dep)) {
      SmallString<64> camel{mod_name};
      size_t p = 0;
      while ((p = camel.find('_', p)) != StringRef::npos) {
        camel[p] = '-';
      }

      dep.clear();
      sys::path::append(dep, "..", camel, t);
    }
    if (!sys::fs::is_regular_file(dep.c_str())) {
      return false;
    }
  }

  return add_real_abs(m_mod_deps, dep);
}
bool dag::node::add_mod_impl(llvm::StringRef mod_impl) {
  return add_real_abs(m_mod_impls, mod_impl);
}
bool dag::node::add_resource(llvm::StringRef resource) {
  return add_real_abs(m_resources, resource);
}
bool dag::node::add_shader(llvm::StringRef shader) {
  return add_real_abs(m_shaders, shader);
}

namespace {
class ppc : public PPCallbacks {
  DiagnosticsEngine *m_diag;
  dag::node *m_dag;

public:
  explicit ppc(DiagnosticsEngine *diag, dag::node *d)
      : m_diag{diag}
      , m_dag{d} {}

  bool FileNotFound(StringRef filename) override { return true; }
  void moduleImport(SourceLocation loc, ModuleIdPath path,
                    const Module *imp) override {
    assert(path.size() == 1 && "path isn't atomic");
    auto &[id, _] = path[0];
    if (!m_dag->add_mod_dep(id->getName())) {
      diag_error(*m_diag, loc, "module not found");
    }
  }
};

class action : public PreprocessOnlyAction {
  dag::node *m_dag;

public:
  explicit action(dag::node *d) : m_dag{d} {}

  bool BeginSourceFileAction(CompilerInstance &ci) override {
    auto *diag = &ci.getDiagnostics();
    ci.getPreprocessor().addPPCallbacks(std::make_unique<ppc>(diag, m_dag));
    ci.getPreprocessor().AddPragmaHandler(new ns_pragma(m_dag));
    return true;
  }
  void EndSourceFileAction() override {
    SmallString<128> pwd;
    sys::fs::current_path(pwd); // TODO: check errors
    auto pwd_stem = sys::path::stem(pwd);
    auto file_stem = sys::path::stem(getCurrentFile());
    auto file_ext = sys::path::extension(getCurrentFile());
    auto &pp = getCompilerInstance().getPreprocessor();
    bool root = pp.isInNamedModule() && pp.getNamedModuleName() == pwd_stem &&
                file_ext == ".cppm";
    if (root && !m_dag->root())
      m_dag->set_main_mod();
  }
};
} // namespace

static StringMap<dag::node> cache{};
void dag::clear_cache() { cache.clear(); }

void dag::errlog(const dag::node *n, const char *msg) {
  errs() << msg << " " << n->source() << "\n";
}
void dag::xlog(const dag::node *n, const char *msg) {
  if (!is_extra_verbose())
    return;
  dag::errlog(n, msg);
}

static bool compile(dag::node *n) {
  auto dag_mtime = mtime_of(n->dag().str().c_str());
  if (dag_mtime > mtime_of(n->source().str().c_str()) &&
      dag_mtime > mtime_of("../leco/leco.exe")) {
    // Capture both success and failures. Failures might leave inconsistencies
    // in the node, which would be worse if we read and store it again
    return n->read_from_cache_file();
  }

  dag::xlog(n, "dag compilation");

  auto ci = evoker{}
                .set_cpp_std()
                .push_arg("-E")
                .push_arg(n->source())
                .add_predefs()
                .createCI();

  action a{n};
  if (!ci->ExecuteAction(a))
    return false;

  n->write_to_cache_file();
  return true;
}

static auto find(StringRef path) {
  dag::node n{path};

  auto [it, inserted] = cache.try_emplace(n.source(), n.source());
  auto *ptr = &(*it).second;

  struct res {
    dag::node *n;
    bool i;
  };
  return res{ptr, inserted};
}

static bool recurse(dag::node *n) {
  for (auto &dep : n->mod_deps()) {
    if (dep.first() == n->source()) {
      errs() << "Self-referencing detected - are you using `import "
                "<mod>:<part>` instead of `import :<part>`?\n";
      return false;
    }

    auto [d, ins] = find(dep.first());

    if (!d)
      return false;
    if (d->recursed())
      continue;
    if (!compile(d))
      return false;
    if (!recurse(d))
      return false;

    d->set_recursed();
  }
  for (auto &impl : n->mod_impls()) {
    auto [d, ins] = find(impl.first());

    if (!d)
      return false;
    if (d->recursed())
      continue;

    if (sys::path::extension(impl.first()) == ".cpp") {
      if (!compile(d))
        return false;
      if (!recurse(d))
        return false;
      // TODO: do we care if we have a CPP file which isn't a C++20 impl unit?
      if (!d->add_mod_dep(n->module_name()))
        return false;
    }

    d->set_recursed();
  }
  return true;
}

dag::node *dag::get_node(StringRef source) {
  auto it = cache.find(source);
  return it == cache.end() ? nullptr : &(*it).second;
}
dag::node *dag::process(StringRef path) {
  auto [n, ins] = find(path);
  if (!n || !ins)
    return n;
  if (!compile(n))
    return nullptr;
  if (!n->root())
    return n;

  return recurse(n) ? n : nullptr;
}

uint64_t dag::visit_dirty(const node *n, function_ref<void(const node *)> fn) {
  StringMap<uint64_t> visited{};

  uint64_t max{};
  const auto rec = [&](auto rec, auto *n, uint64_t pmt) -> uint64_t {
    auto it = visited.find(n->source());
    if (it != visited.end()) {
      max = max > it->second ? max : it->second;
      return it->second;
    }

    auto mtime = mtime_of(n->source().str().c_str());
    mtime = mtime > pmt ? mtime : pmt;

    for (auto &d : n->mod_deps()) {
      auto dmt = rec(rec, get_node(d.first()), {});
      mtime = mtime > dmt ? mtime : dmt;
    }

    if (mtime > mtime_of(n->target().str().c_str())) {
      fn(n);
      // Get the mtime again after `fn` possibly changed it
      mtime = mtime_of(n->target().str().c_str());
    }

    max = max > mtime ? max : mtime;
    visited[n->source()] = mtime;

    for (auto &d : n->mod_impls()) {
      rec(rec, get_node(d.first()), mtime);
    }

    return mtime;
  };
  rec(rec, n, {});
  return max;
}
