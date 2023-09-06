#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define EXE_EXT ".exe"
#else
#define EXE_EXT ""
#endif

#include "clang_dir.hpp"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"

using namespace clang;
using namespace clang::driver;
using namespace llvm;

DiagnosticsEngine &diag_engine() {
  static IntrusiveRefCntPtr<DiagnosticOptions> diag_opts{
      new DiagnosticOptions()};
  static IntrusiveRefCntPtr<DiagnosticIDs> diag_ids{new DiagnosticIDs()};
  static auto diag_cli = new TextDiagnosticPrinter(llvm::errs(), &*diag_opts);

  static DiagnosticsEngine diags{diag_ids, diag_opts, diag_cli};
  return diags;
}

const char *clang_exe() {
  static const auto exe = [] {
    SmallString<1024> buf{};
    sys::path::append(buf, clang_dir(), "bin", "clang++" EXE_EXT);
    return buf;
  }();
  return exe.data();
}

auto cc1(SmallVectorImpl<const char *> args) {
  auto cinst = std::make_unique<CompilerInstance>();
  CompilerInvocation::CreateFromArgs(cinst->getInvocation(), args,
                                     diag_engine(), clang_exe());
  cinst->createDiagnostics();

  EmitObjAction a{};
  return !cinst->ExecuteAction(a);
}

bool compile(StringRef file) {
  errs() << "compiling " << file << "\n";

  auto def_triple = sys::getDefaultTargetTriple();
  Driver drv{clang_exe(), def_triple, diag_engine()};

  auto path = sys::path::parent_path(file);
  auto name = sys::path::stem(file);
  auto ext = sys::path::extension(file);
  SmallString<128> obj{};
  sys::path::append(obj, path, "out", name);

  const char *mode;
  if (ext == ".cppm") {
    mode = "--precompile";
    sys::path::replace_extension(obj, ".pcm");
  } else {
    mode = "-c";
    sys::path::replace_extension(obj, ext + ".o");
  }

  std::vector<const char *> args{};
  args.push_back(clang_exe());
  args.push_back("-std=c++20");
  args.push_back(mode);
  args.push_back(file.data());
  args.push_back("-o");
  args.push_back(obj.data());

  std::unique_ptr<Compilation> c{drv.BuildCompilation(args)};
  if (!c || c->containsError())
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return false;

  SmallVector<std::pair<int, const ::clang::driver::Command *>, 4> failures;
  int res = drv.ExecuteCompilation(*c, failures);
  for (const auto &p : failures) {
    if (p.first)
      return false;
  }
  return res == 0;
}
