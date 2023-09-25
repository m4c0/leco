#pragma once
#include "llvm/ADT/StringRef.h"

void gen_info_plist(llvm::StringRef exe_path, llvm::StringRef name);
void gen_archive_plist(llvm::StringRef build_path, llvm::StringRef name);
void gen_export_plist(llvm::StringRef build_path, llvm::StringRef name);
