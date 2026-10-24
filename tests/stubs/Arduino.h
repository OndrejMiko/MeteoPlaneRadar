#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
inline unsigned long millis() { return 10000; }
struct TestSerial {
  template <typename... Args> void printf(const char*, Args...) {}
};
inline TestSerial Serial;
