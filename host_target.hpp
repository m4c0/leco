#pragma once

#if _WIN32
#define HOST_TARGET "x86_64-pc-windows-msvc"
#elif __APPLE__
#define HOST_TARGET "x86_64-apple-macosx11.6.0"
#elif __linux__
#define HOST_TARGET "x86_64-pc-linux-gnu"
#endif
