#include "NotificationPairing.h"
#include <stdio.h>
#include <string.h>

const char* NotificationPairing::begin(uint32_t now, uint32_t random) {
  snprintf(code_, sizeof(code_), "%06lu", (unsigned long)(random % 1000000));
  started_ = now; attempts_ = 0; open_ = true; completed_ = false;
  return code_;
}
bool NotificationPairing::active(uint32_t now) {
  if (open_ && (uint32_t)(now - started_) >= TTL_MS) cancel();
  return open_;
}
bool NotificationPairing::take(const char* code, uint32_t now) {
  if (!active(now)) return false;
  ++attempts_;
  unsigned diff = !code || strlen(code) != 6;
  if (!diff) for (unsigned i = 0; i < 6; ++i) diff |= (unsigned char)code[i] ^ (unsigned char)code_[i];
  if (!diff) { cancel(); completed_ = true; return true; }
  if (attempts_ >= MAX_ATTEMPTS) cancel();
  return false;
}
void NotificationPairing::cancel() {
  memset(code_, 0, sizeof(code_)); open_ = false; attempts_ = 0; completed_ = false;
}
