#pragma once
#include <map>
#include <string>
#include <algorithm>
#include <cstring>

// In-memory NVS shared across Settings_Begin calls to simulate reboot.
class Preferences {
 public:
  inline static std::map<std::string, double> numbers;
  inline static std::map<std::string, std::string> strings;
  bool begin(const char*, bool) { return true; }
  void end() {}
  bool isKey(const char* k) { return numbers.count(k) || strings.count(k); }
  double getDouble(const char* k, double fallback) { return numbers.count(k) ? numbers[k] : fallback; }
  uint8_t getUChar(const char* k, uint8_t fallback) { return getDouble(k, fallback); }
  uint16_t getUShort(const char* k, uint16_t fallback) { return getDouble(k, fallback); }
  int8_t getChar(const char* k, int8_t fallback) { return getDouble(k, fallback); }
  bool getBool(const char* k, bool fallback) { return getDouble(k, fallback) != 0; }
  size_t getString(const char* k, char* out, size_t cap) {
    if (!strings.count(k) || !cap) return 0;
    const auto n = std::min(cap - 1, strings[k].size());
    memcpy(out, strings[k].data(), n); out[n] = 0; return n;
  }
  void putDouble(const char* k, double v) { numbers[k] = v; }
  void putUChar(const char* k, uint8_t v) { numbers[k] = v; }
  void putUShort(const char* k, uint16_t v) { numbers[k] = v; }
  void putChar(const char* k, int8_t v) { numbers[k] = v; }
  void putBool(const char* k, bool v) { numbers[k] = v; }
  void putString(const char* k, const char* v) { strings[k] = v; }
  void remove(const char* k) { numbers.erase(k); strings.erase(k); }
  void clear() { numbers.clear(); strings.clear(); }
};
