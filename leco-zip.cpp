#pragma leco tool
import sys;

static void usage() {
  die(R"(
Creates ZIP files out of each "app". Useful for distribution of Windows and
WASM targets.

Usage: ../leco/leco.exe zip
)");
}

static void run(const char * dag, const char * _) {
  auto path = sim::sb { dag }.path_extension("app");
  auto zip = sim::sb { dag }.path_extension("zip");

  sys::log("bundling", *zip);
  // TODO: use DAG to find which files should be bundled
  sys::runf("zip -q -j %s %s/*", *zip, *path);
}

int main(int argc, char **argv) try {
  if (sys::is_tgt_apple()) usage();
  sys::for_each_tag_in_dags('tapp', false, &run);
} catch (...) {
  return 1;
}
