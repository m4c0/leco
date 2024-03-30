#pragma once
#include "context.hpp"
#include "sim.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

static void in2out(const sim_sb *in, sim_sb *out, const char *ext) {
  sim_sb_copy(out, in->buffer);
  if (strcmp(in->buffer, SIM_PATHSEP_S "out" SIM_PATHSEP_S) == 0) {
    sim_sb_path_parent(out);
    sim_sb_path_append(out, "out");
    sim_sb_path_append(out, cur_ctx().target.c_str());
    sim_sb_path_append(out, sim_sb_path_filename(in));
  }
  sim_sb_path_set_extension(out, ext);
}

static void in2out(llvm::StringRef in, llvm::SmallVectorImpl<char> &out,
                   llvm::StringRef ext) {
  out.clear();

  auto path = llvm::sys::path::parent_path(in);
  auto gpath = llvm::sys::path::parent_path(path);
  if (llvm::sys::path::stem(gpath) != "out") {
    auto name = llvm::sys::path::filename(in);
    auto triple = cur_ctx().target;
    llvm::sys::path::append(out, path, "out", triple, name);
  } else {
    llvm::sys::path::append(out, in);
  }
  // TODO: check errors
  llvm::sys::fs::make_absolute(out);
  llvm::sys::path::replace_extension(out, ext);
}
