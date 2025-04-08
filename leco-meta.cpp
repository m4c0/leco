#pragma leco tool
import mtime;
import sim;
import sys;
import sysstd;

// Using Windows definition for extra constness
extern "C" int execv(const char *, char * const *);

static void chdir(const char * dir) {
  if (0 != sysstd::chdir(dir)) sys::die("Directory not found: [%s]\n", dir);
}

extern "C" int puts(const char *);
int main(int argc, char ** argv) try {
  if (argv[argc]) return 69;

#ifdef _WIN32
  system(""); // enable ANSI colours
#endif

  const auto shift = [&] { return argc ? (argc--, *++argv) : nullptr; };
  while (auto val = shift())
    if ("-C"_s == val) chdir(shift());
    else break;

  auto cmd = sys::tool_cmd(*argv ? *argv : "");
  if (!mtime::of(*cmd)) {
    argv--;
    cmd = sys::tool_cmd("driver");
  }
  *argv = *cmd;
  execv(*argv, argv);
} catch (...) {
  return 1;
}
