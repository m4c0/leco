#ifndef PATHMAX_H
#define PATHMAX_H

#ifdef __linux__
#include <linux/limits.h>
#elif _WIN32
#include <stdlib.h>
#define PATH_MAX _MAX_PATH
#else
#include <limits.h>
#endif

#endif
