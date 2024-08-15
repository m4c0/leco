#pragma leco tool

extern "C" int system(const char *);

int main(int argc, char ** argv) { return system("python3 ../leco/webserver.py"); }
