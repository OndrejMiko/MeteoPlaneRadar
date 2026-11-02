#include "NotificationPairing.h"
#include <cassert>
#include <cstring>
#include <iostream>
int main() {
  NotificationPairing p;
  assert(!p.take("123456", 0));
  assert(!strcmp(p.begin(100, 1234), "001234"));
  assert(p.active(120099));
  assert(p.take("001234", 120099));
  assert(!p.take("001234", 120099));  // consumed once
  assert(p.completed());
  p.begin(100, 1234);
  assert(!p.completed());
  assert(!p.active(120100));  // expiry includes exact boundary
  assert(!p.take("001234", 120100));
  p.begin(0, 1234);
  for (int i = 0; i < 4; ++i) assert(!p.take("000000", 1));
  assert(p.take("001234", 2));  // fifth attempt may be correct
  p.begin(0, 1234);
  for (int i = 0; i < 5; ++i) assert(!p.take("000000", 1));
  assert(!p.take("001234", 2));
  p.begin(0, 1234);
  assert(!p.take(nullptr, 1)); assert(!p.take("001234extra", 1));
  assert(!p.take("01234", 1)); assert(!p.take("abc123", 1));
  assert(p.take("001234", 2));
  p.begin(10, 111111); p.begin(20, 222222);
  assert(!p.take("111111", 21)); assert(p.take("222222", 22));
  p.begin(0, 0); p.cancel(); assert(!p.active(1)); assert(!p.completed());
  assert(!strcmp(p.begin(0xfffffff0u, 4294967295u), "967295"));
  assert(p.active(0xfffffff0u + 119999u));
  assert(!p.active(0xfffffff0u + 120000u));
  std::cout << "Pairing code single-use, expiry, attempt budget, replacement and millis wrap passed\n";
}
