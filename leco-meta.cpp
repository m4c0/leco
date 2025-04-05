#pragma leco tool
import mtime;
import sim;
import sys;
import sysstd;

static void chdir(const char * dir) {
  if (0 != sysstd::chdir(dir)) sys::die("Directory not found: [%s]\n", dir);
}

int main(int argc, char ** argv) try {
#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  const auto shift = [&] { return argc ? "" : (argc--, *++argv); };
  argc--;

  if (sim::sb{"-C"} == *argv) chdir((shift(), shift()));

  auto cmd = sys::tool_cmd(argc && mtime::of(*argv) ? shift() : "driver");
  while (argc) cmd.printf(" %s", shift());
  sys::run(*cmd);
} catch (...) {
  return 1;
}
