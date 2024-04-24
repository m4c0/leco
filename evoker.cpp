#include "evoker.hpp"

#include "../tempsie/tempsie.h"
#include "cl.hpp"
#include "clang_dir.hpp"
#include "context.hpp"
#include "dag.hpp"
#include "sim.hpp"

#include <stdio.h>

#ifndef _WIN32
#include <unistd.h>
#endif

static void construct_args(const char *in, std::vector<std::string> &args) {
  auto ext = sim_path_extension(in);
  if (0 == strcmp(ext, ".c")) {
    args.push_back(clang_c_exe());
  } else {
    args.push_back(clang_cpp_exe());
  }

  args.push_back("-Wall");
  args.push_back("-target");
  args.push_back(cur_ctx().target.c_str());

  if (is_optimised()) {
    args.push_back("-O3");
    args.push_back("-flto");
    args.push_back("-fvisibility=hidden");
  }
  if (enable_debug_syms()) {
    args.push_back("-g");
  }

  if (cur_ctx().sysroot != "") {
    args.push_back("--sysroot");
    args.push_back(cur_ctx().sysroot.c_str());
  }

  for (auto f : cur_ctx().cxx_flags) {
    args.push_back(f);
  }
}

evoker::evoker() { construct_args("null.cpp", m_args); }
evoker::evoker(const char *verb, const char *in, const char *out) {
  construct_args(in, m_args);
  push_arg(verb);
  push_arg(in);
  set_out(out);
}

static void out_file(FILE *f, const char *a) {
  while (*a != 0) {
    char c = *a++;
    if (c == '\\') {
      fputs("\\\\", f); // escapes backslash
    } else {
      fputc(c, f);
    }
  }
  fputc('\n', f);
}
static std::string create_args_file(const auto &args, const dag::node *node) {
  char file[1024];
  if (0 != tempsie_get_temp_filename("leco", file, sizeof(file)))
    return "";

  FILE *f = fopen(file, "w");
  if (f == nullptr)
    return "";

  bool first{true};
  for (const auto &a : args) {
    if (first) {
      first = false;
      continue;
    }
    out_file(f, a.c_str());
  }

  if (node != nullptr) {
    dag::visit(node, false, [&](auto *n) {
      fprintf(f, "-fmodule-file=%s=", n->module_name());
      out_file(f, n->module_pcm());
    });
  }
  fclose(f);

  return file;
}
evoker &evoker::add_predefs() {
  for (auto def : cur_ctx().predefs) {
    m_args.push_back(std::string{"-D"} + def);
  }
  return *this;
}
vex evoker::prepare_args() { return vex{create_args_file(m_args, m_node)}; }
bool evoker::execute() {
  vex v = prepare_args();
  if (!v)
    return false;

  if (is_extra_verbose()) {
    fprintf(stderr, "executing with argument file [%s]\n", v.argument_file());
  }

  sim_sbt cmd{};
  sim_sb_copy(&cmd, m_args[0].c_str());
  sim_sb_concat(&cmd, " @");
  sim_sb_concat(&cmd, v.argument_file());
  return 0 == system(cmd.buffer);
}

evoker &evoker::suppress_pragmas() { return push_arg("-Wno-unknown-pragmas"); }

evoker &evoker::set_cpp() {
  return push_arg("-std=c++2b").add_predefs().suppress_pragmas();
}

#ifdef _WIN32
#define unlink _unlink
#endif
vex::~vex() { unlink(m_argfile.c_str()); }
vex::operator bool() const { return m_argfile != ""; }
