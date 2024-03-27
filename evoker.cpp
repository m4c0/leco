#include "evoker.hpp"

#include "../tempsie/tempsie.h"
#include "cl.hpp"
#include "clang_dir.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "sim.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include <fstream>
#include <unistd.h>

using namespace clang;
using namespace clang::driver;
using namespace llvm;

const char *clang_exe() {
  static const auto exe = [] {
    sim_sb buf{};
    sim_sb_new(&buf, 1024);
    sim_sb_copy(&buf, clang_dir());
    sim_sb_path_append(&buf, "bin");
#ifdef _WIN32
    sim_sb_path_append(&buf, "clang++.exe");
#else
    sim_sb_path_append(&buf, "clang++");
#endif
    return buf;
  }();
  return exe.buffer;
}

static DiagnosticsEngine diags() {
  IntrusiveRefCntPtr<DiagnosticOptions> diag_opts{new DiagnosticOptions()};
  IntrusiveRefCntPtr<DiagnosticIDs> diag_ids{new DiagnosticIDs()};
  auto diag_cli = new TextDiagnosticPrinter(errs(), &*diag_opts);

  return DiagnosticsEngine{diag_ids, diag_opts, diag_cli};
}

static std::vector<const char *> prepare_args(const auto &margs,
                                              const char *verb) {
  std::vector<const char *> args{};
  for (const auto &s : margs) {
    args.push_back(s.c_str());
  }
  if (is_extra_verbose()) {
    auto tgt = cur_ctx().target;
    errs() << verb << " compiler instance for args (target = [" << tgt
           << "]):\n";
    for (auto a : args)
      errs() << a << " ";
    errs() << "\n";
  }
  return args;
}

static std::shared_ptr<CompilerInstance> createCI(const auto &margs) {
  auto clang = std::make_shared<CompilerInstance>();

  auto args = prepare_args(margs, "preparing");
  auto diags = ::diags();
  Driver drv{clang_exe(), cur_ctx().target, diags};
  std::unique_ptr<Compilation> c{drv.BuildCompilation(args)};
  if (!c || c->containsError() || c->getJobs().size() == 0)
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return {};

  auto cc1args = c->getJobs().begin()->getArguments();
  if (!CompilerInvocation::CreateFromArgs(clang->getInvocation(), cc1args,
                                          diags))
    return {};

  clang->createDiagnostics();
  return clang;
}

evoker::evoker() {
  m_args.push_back(clang_exe());
  m_args.push_back("-Wall");
  m_args.push_back("-target");
  m_args.push_back(cur_ctx().target.c_str());

  if (is_optimised()) {
    m_args.push_back("-O3");
    m_args.push_back("-flto");
    m_args.push_back("-fvisibility=hidden");
  }
  if (enable_debug_syms()) {
    m_args.push_back("-g");
  }

  if (cur_ctx().sysroot != "") {
    m_args.push_back("--sysroot");
    m_args.push_back(cur_ctx().sysroot.c_str());
  }

  for (auto f : cur_ctx().cxx_flags) {
    m_args.push_back(f.str());
  }
}

std::shared_ptr<CompilerInstance> evoker::createCI() const {
  auto ci = ::createCI(m_args);

  if (m_node == nullptr)
    return ci;

  auto &mod_files = ci->getHeaderSearchOpts().PrebuiltModuleFiles;
  dag::visit(m_node, false, [&](auto *n) {
    mod_files.insert({n->module_name().str(), n->module_pcm().str()});
  });

  return ci;
}

static void out_file(auto &f, const auto &a) {
  for (auto c : a) {
    if (c == '\\')
      f << '\\';
    f << c;
  }
}
static std::string create_args_file(const auto &args, const dag::node *node) {
  char file[1024];
  if (0 != tempsie_get_temp_filename("leco", file, sizeof(file)))
    return "";

  std::ofstream f{file};
  bool first{true};
  for (const auto &a : args) {
    if (first) {
      first = false;
      continue;
    }
    out_file(f, a);
    f << "\n";
  }

  if (node != nullptr) {
    dag::visit(node, false, [&](auto *n) {
      f << "-fmodule-file=" << n->module_name().str() << "=";
      out_file(f, n->module_pcm().str());
      f << "\n";
    });
  }

  return file;
}
#ifdef _WIN32
#define unlink _unlink
#endif
evoker &evoker::add_predefs() {
  for (auto def : cur_ctx().predefs) {
    push_arg(std::string{"-D"} + def.str());
  }
  return *this;
}
bool evoker::execute() {
  auto argfile = create_args_file(m_args, m_node);
  if (argfile == "")
    return false;
  std::string cmd = std::string{clang_exe()} + " @" + argfile;
  if (is_extra_verbose()) {
    errs() << "executing [" << cmd << "]\n";
  }
  if (system(cmd.c_str()) != 0) {
    return false;
  }
  unlink(argfile.c_str());
  return true;
}

struct init_llvm {
  llvm_shutdown_obj sdo{};

  init_llvm() {
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();
    CrashRecoveryContext::Enable();
  }
} i{};
