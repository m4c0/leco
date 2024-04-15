#pragma once
struct context;

namespace t {
context android_aarch64();
context android_armv7();
context android_i686();
context android_x86_64();
context iphoneos();
context iphonesimulator();
context linux();
context macosx();
context windows();
} // namespace t
