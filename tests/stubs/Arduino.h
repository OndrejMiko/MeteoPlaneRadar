#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
inline unsigned long testMillis=10000;
inline unsigned long millis() { return testMillis; }
struct TestSerial {
  template <typename... Args> void printf(const char*, Args...) {}
};
inline TestSerial Serial;

// Network fixture only needs the String type in Net.h declarations.
using String = std::string;

#ifndef PROGMEM
#define PROGMEM
#endif
