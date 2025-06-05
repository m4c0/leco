#pragma leco tool

import sys;

static void bundle(const char * dag, const char * _) {
  if (sys::is_tgt_apple()) sys::tool_run("ipa", "-i %s", dag);
}

int main() try {
  if (sys::is_tgt_wasm()) sys::tool_run("wasm-js");

  sys::for_each_tag_in_dags('tapp', false, &bundle);
  return 0;
} catch (...) {
  return 1;
}
