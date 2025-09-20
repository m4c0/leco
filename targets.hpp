#pragma once
#ifdef __arm64__
#define TGT_OSX "arm64-apple-macosx14"
#else
#define TGT_OSX "x86_64-apple-macosx11.6.0"
#endif
#define TGT_IPHONEOS "arm64-apple-ios17.0"
#define TGT_IOS_SIMULATOR "arm64-apple-ios17.0-simulator"
#define TGT_WINDOWS "x86_64-pc-windows-msvc"
#define TGT_LINUX "x86_64-pc-linux-gnu"
#define TGT_DROID_AARCH64 "aarch64-none-linux-android26"
#define TGT_DROID_ARMV7 "armv7-none-linux-androideabi26"
#define TGT_DROID_X86 "i686-none-linux-android26"
#define TGT_DROID_X86_64 "x86_64-none-linux-android26"
#define TGT_WASM "wasm32-wasi"

#if _WIN32
#define HOST_TARGET TGT_WINDOWS
#elif __APPLE__
#define HOST_TARGET TGT_OSX
#elif __linux__
#define HOST_TARGET TGT_LINUX
#endif

#if __APPLE__ && !__arm64__
#define CLANG_CMD "/usr/local/opt/llvm/bin/clang++"
#elif __APPLE__
#define CLANG_CMD "/opt/homebrew/opt/llvm/bin/clang++"
#elif _WIN32
#define CLANG_CMD "clang++.exe"
#else
#define CLANG_CMD "clang++"
#endif
