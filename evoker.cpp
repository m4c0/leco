#ifdef _WIN32
#define EXE_EXT ".exe"
#else
#define EXE_EXT ""
#endif

#include "cl.hpp"
#include "clang_dir.hpp"
#include "diags.hpp"
#include "evoker.hpp"
#include "instance.hpp"
#include "clang/CodeGen/ObjectFilePCHContainerOperations.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"

using namespace clang;
using namespace clang::driver;
using namespace llvm;

const char *clang_exe() {
  static const auto exe = [] {
    SmallString<1024> buf{};
    sys::path::append(buf, clang_dir(), "bin", "clang++" EXE_EXT);
    return buf;
  }();
  return exe.data();
}

std::shared_ptr<CompilerInstance> createCI(ArrayRef<const char *> args) {
  auto clang = std::make_shared<CompilerInstance>();

  Driver drv{clang_exe(), current_target(), diags()};
  std::unique_ptr<Compilation> c{drv.BuildCompilation(args)};
  if (!c || c->containsError() || c->getJobs().size() == 0)
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return {};

  auto cc1args = c->getJobs().begin()->getArguments();
  if (!CompilerInvocation::CreateFromArgs(clang->getInvocation(), cc1args,
                                          diags()))
    return {};

  clang->createDiagnostics();
  return clang;
}

void in2out(llvm::StringRef in, llvm::SmallVectorImpl<char> &out,
            llvm::StringRef ext) {
  out.clear();

  auto path = sys::path::parent_path(in);
  auto gpath = sys::path::parent_path(path);
  if (sys::path::stem(gpath) != "out") {
    auto name = sys::path::stem(in);
    auto triple = current_target();
    sys::path::append(out, path, "out", triple, name);
  } else {
    sys::path::append(out, in);
  }
  // TODO: check errors
  sys::fs::make_absolute(out);
  sys::path::replace_extension(out, ext);
}

evoker::evoker() {
  m_args.push_back(clang_exe());
  m_args.push_back("-std=c++20");
  m_args.push_back("-target");
  m_args.push_back(current_target());
}
evoker &evoker::set_inout(StringRef in, StringRef ext) {
  in2out(in, m_obj, ext);

  m_args.push_back(in.data());
  m_args.push_back("-o");
  m_args.push_back(m_obj.c_str());
  return *this;
}
instance evoker::build() { return instance{createCI(m_args), m_obj.str()}; }

bool evoker::execute() {
  Driver drv{clang_exe(), current_target(), diags()};
  std::unique_ptr<Compilation> c{drv.BuildCompilation(m_args)};
  if (!c || c->containsError() || c->getJobs().size() == 0)
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return {};

  SmallVector<std::pair<int, const Command *>, 4> fails;
  auto res = drv.ExecuteCompilation(*c, fails);
  for (auto &p : fails) {
    if (!res)
      res = p.first;
  }
  return res;
}
