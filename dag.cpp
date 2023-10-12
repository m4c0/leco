#include "dag.hpp"
#include "evoker.hpp"
#include "pragma.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/FileSystem.h"

using namespace clang;
using namespace llvm;

dag::node::node(StringRef n) : m_source{} {
  sys::fs::real_path(n, m_source);
  sys::fs::make_absolute(m_source);
}
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
  m_mod_deps.insert(dep);
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
dag::node *dag::process(StringRef path) {
  node n{path};

  auto [it, inserted] = cache.try_emplace(n.source(), n);
  auto *ptr = &(*it).second;
  if (!inserted) {
    return ptr;
  }

  auto ci =
      evoker{}.set_cpp_std().push_arg("-E").push_arg(n.source()).createCI();
  // ci->getDiagnostics().setClient(new IgnoringDiagConsumer());

  action a{ptr};
  if (!ci->ExecuteAction(a))
    return {};

  return ptr;
}
