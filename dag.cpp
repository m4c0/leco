#include "dag.hpp"
#include "evoker.hpp"
#include "pragma.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/FileSystem.h"

using namespace clang;
using namespace llvm;

static void real_abs(SmallVectorImpl<char> &buf, StringRef path) {
  sys::fs::real_path(path, buf);
  sys::fs::make_absolute(buf);
}

dag::node::node(StringRef n) : m_source{} { real_abs(m_source, n); }
void dag::node::add_mod_dep(llvm::StringRef mod_name) {
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
      return;
    }
  }

  SmallString<256> abs{};
  real_abs(abs, dep);
  m_mod_deps.insert(abs);
}

namespace {
class ppc : public PPCallbacks {
  dag::node *m_dag;

public:
  explicit ppc(dag::node *d) : m_dag{d} {}

  void moduleImport(SourceLocation loc, ModuleIdPath path,
                    const Module *imp) override {
    assert(path.size() == 1 && "path isn't atomic");
    auto &[id, _] = path[0];
    m_dag->add_mod_dep(id->getName());
  }
};

class action : public PreprocessOnlyAction {
  dag::node *m_dag;

public:
  explicit action(dag::node *d) : m_dag{d} {}

  bool BeginSourceFileAction(CompilerInstance &ci) override {
    ci.getPreprocessor().addPPCallbacks(std::make_unique<ppc>(m_dag));
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
    if (root)
      m_dag->set_root();
  }
};
} // namespace

static StringMap<dag::node> cache{};

static bool compile(dag::node *n) {
  auto ci =
      evoker{}.set_cpp_std().push_arg("-E").push_arg(n->source()).createCI();
  // ci->getDiagnostics().setClient(new IgnoringDiagConsumer());

  action a{n};
  return ci->ExecuteAction(a);
}

static auto find(StringRef path) {
  dag::node n{path};

  auto [it, inserted] = cache.try_emplace(n.source(), n);
  auto *ptr = &(*it).second;

  struct res {
    dag::node *n;
    bool i;
  };
  return res{ptr, inserted};
}

static bool recurse(dag::node *n) {
  for (auto &dep : n->mod_deps()) {
    auto [d, ins] = find(dep.first());

    if (!d)
      return false;
    if (!ins)
      continue;
    if (!compile(d))
      return false;
    if (!recurse(d))
      return false;
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
