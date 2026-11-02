#pragma once
#include <stdint.h>

// One RAM-only, admin-opened pairing window. No key rotation until a correct
// code is consumed; five guesses close the window. Unsigned elapsed time wraps.
class NotificationPairing {
 public:
  static constexpr uint32_t TTL_MS = 120000;
  static constexpr uint8_t MAX_ATTEMPTS = 5;
  const char* begin(uint32_t now, uint32_t random);
  bool active(uint32_t now);
  bool take(const char* code, uint32_t now);
  bool completed() const { return completed_; }
  void cancel();
 private:
  char code_[7] = {};
  uint32_t started_ = 0;
  uint8_t attempts_ = 0;
  bool open_ = false;
  bool completed_ = false;
};
