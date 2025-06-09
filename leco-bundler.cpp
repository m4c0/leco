#pragma leco tool

import sys;

int main() try {
  if (sys::is_tgt_apple()) sys::tool_run("ipa");
  if (sys::is_tgt_wasm()) sys::tool_run("wasm-js");
  return 0;
} catch (...) {
  return 1;
}
