#pragma once
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

static void in2out(llvm::StringRef in, llvm::SmallVectorImpl<char> &out) {
  out.clear();

  auto path = llvm::sys::path::parent_path(in);
  auto gpath = llvm::sys::path::parent_path(path);
  if (llvm::sys::path::stem(gpath) != "out") {
    auto name = llvm::sys::path::stem(in);
    auto triple = cur_ctx().target;
    llvm::sys::path::append(out, path, "out", triple, name);
  } else {
    llvm::sys::path::append(out, in);
  }
  // TODO: check errors
  llvm::sys::fs::make_absolute(out);
}
static void in2out(llvm::StringRef in, llvm::SmallVectorImpl<char> &out,
                   llvm::StringRef ext) {
  in2out(in, out);
  llvm::sys::path::replace_extension(out, ext);
}
