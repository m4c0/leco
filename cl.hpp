#pragma once

bool should_clean_current();
bool should_clean_all();

bool is_verbose();

bool enable_debug_syms();
bool is_optimised();

bool for_each_target(bool (*)());

void parse_args(int argc, char **argv);
