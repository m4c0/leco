#pragma once
struct sim_sb;

void bundle(const char *dag);

bool actool(const char *path);
void gen_iphone_plists(const char *exe_path, const char *name);

bool find_android_llvm(sim_sb *out);
