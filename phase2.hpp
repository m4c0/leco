#pragma once
namespace dag {
class node;
}
struct sim_sb;

void bundle(const dag::node *n);
void copy_build_deps(const dag::node *n);

bool actool(const char *path);
void gen_iphone_plists(const char *exe_path, const char *name);

bool find_android_llvm(sim_sb *out);
