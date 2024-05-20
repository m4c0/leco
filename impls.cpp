#define GOPT_IMPLEMENTATION
#define PPRENT_IMPLEMENTATION
#define MKDIR_IMPLEMENTATION
#define MTIME_IMPLEMENTATION
#define POPEN_IMPLEMENTATION
#define SIM_IMPLEMENTATION
#define TEMPSIE_IMPLEMENTATION
#define TEMPSIE_ERROR(x) fprintf(stderr, "error: %s\n", (x))

#include "../gopt/gopt.h"
#include "../mtime/mtime.h"
#include "../popen/popen.h"
#include "../pprent/pprent.hpp"
#include "../tempsie/tempsie.h"
#include "mkdir.h"
#include "sim.h"
