#pragma once

bool should_clean_current();
bool should_clean_all();

bool is_verbose();
bool is_extra_verbose();

bool enable_debug_syms();
bool is_optimised();

bool is_dumping_dag();

bool for_each_target(bool (*)());

bool parse_args(int argc, char **argv);
