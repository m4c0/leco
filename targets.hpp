#pragma once
#define TGT_OSX "x86_64-apple-macosx11.6.0"
#define TGT_IPHONEOS "arm64-apple-ios16.1"
#define TGT_IOS_SIMULATOR "x86_64-apple-ios16.1-simulator"
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

#define IS_TGT(t, x) (0 == strcmp((t), (x)))
#define IS_TGT_DROID(t)                                                        \
  (IS_TGT(t, TGT_DROID_AARCH64) || IS_TGT(t, TGT_DROID_ARMV7) ||               \
   IS_TGT(t, TGT_DROID_X86) || IS_TGT(t, TGT_DROID_X86_64))
#define IS_TGT_IOS(t) (IS_TGT(t, TGT_IPHONEOS) || IS_TGT(t, TGT_IOS_SIMULATOR))
#define IS_TGT_APPLE(t) (IS_TGT(t, TGT_OSX) || IS_TGT_IOS(t))
