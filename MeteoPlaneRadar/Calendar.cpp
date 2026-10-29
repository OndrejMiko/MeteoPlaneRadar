#include "Calendar.h"

#include <WiFi.h>
#include <esp_heap_caps.h>
#include <stdlib.h>
#include <string.h>

#include "Lang.h"
#include "Net.h"
#include "Outside.h"
#include "Settings.h"
#include "Watchdog.h"

namespace {
struct Cache {
  CalendarParseDiagnostics diagnostics;
  CalendarEvent events[CALENDAR_EVENTS_PER_SOURCE];
  size_t count;
  uint64_t sourceHash;
  time_t updated;
  CalendarSourceState state;
  unsigned long attempted;
  bool tried;
};
Cache* cache = nullptr;
CalendarEvent* staging = nullptr;
uint8_t* feed = nullptr;
void* scratch = nullptr;
uint32_t revision = 0, generation = 1;
time_t windowStart = 0, windowEnd = 0;
bool busy = false, refresh = false, memoryFailed = false;
uint8_t cursor = 0;
size_t memoryBytes = 0;
unsigned long memoryAttempt = 0;
void release() {
  free(cache);
  free(staging);
  free(feed);
  free(scratch);
  cache = nullptr;
  staging = nullptr;
  feed = nullptr;
  scratch = nullptr;
  memoryBytes = 0;
}
bool allocate() {
  if (cache) return true;
  // Do not compete with TLS for internal RAM and never fall back there.
  cache = (Cache*)heap_caps_calloc(CALENDAR_SOURCES, sizeof(Cache),
                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  staging = (CalendarEvent*)heap_caps_malloc(sizeof(CalendarEvent) * CALENDAR_EVENTS_PER_SOURCE,
                                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  feed = (uint8_t*)heap_caps_malloc(CALENDAR_MAX_FEED + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  scratch = heap_caps_malloc(CalendarIcs_WorkspaceSize(), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!cache || !staging || !feed || !scratch) {
    bool firstFailure = !memoryFailed;
    release();
    memoryFailed = true;
    if (firstFailure) generation++;
    return false;
  }
  memoryFailed = false;
  memoryBytes = CALENDAR_SOURCES * sizeof(Cache) +
                sizeof(CalendarEvent) * CALENDAR_EVENTS_PER_SOURCE + CALENDAR_MAX_FEED + 1 +
                CalendarIcs_WorkspaceSize();
  return true;
}
void setWindow(time_t now) {
  struct tm t{};
  localtime_r(&now, &t);
  t.tm_mday = 1;
  t.tm_hour = 0;
  t.tm_min = 0;
  t.tm_sec = 0;
  t.tm_isdst = -1;
  t.tm_mon--;
  windowStart = mktime(&t);
  t.tm_mon += 4;
  t.tm_isdst = -1;
  windowEnd = mktime(&t);
}
uint64_t sourceHash(int index) {
  const auto& c = Settings_CalendarSource(index);
  uint64_t h = c.enabled ? 14695981039346656037ULL : 0;
  for (const unsigned char* p = (const unsigned char*)c.url; *p; p++)
    h = (h ^ *p) * 1099511628211ULL;
  return h;
}
void syncSources(bool windowChanged) {
  if (!cache) return;
  for (int i = 0; i < CALENDAR_SOURCES; i++) {
    uint64_t hash = sourceHash(i);
    if (cache[i].sourceHash != hash) {
      cache[i] = {};
      cache[i].sourceHash = hash;
      cache[i].state = CAL_WAIT;
    }
    // Keep overlapping old instances while a new month is being fetched.
    if (windowChanged) {
      cache[i].tried = false;
      cache[i].state = CAL_WAIT;
    }
  }
  revision = Settings_CalendarRevision();
  generation++;
}
void parsePoll() {
  Net_Poll();
  Watchdog_Feed();
}
int order(const void* aa, const void* bb) {
  const auto& a = *(const CalendarEvent*)aa;
  const auto& b = *(const CalendarEvent*)bb;
  if (a.start != b.start) return a.start < b.start ? -1 : 1;
  return strcmp(a.title, b.title);
}
bool dayRange(time_t day, time_t& a, time_t& b) {
  if (!day) return false;
  struct tm t{};
  localtime_r(&day, &t);
  t.tm_hour = t.tm_min = t.tm_sec = 0;
  t.tm_isdst = -1;
  a = mktime(&t);
  t.tm_mday++;
  t.tm_isdst = -1;
  b = mktime(&t);
  return true;
}
}  // namespace
void Calendar_Refresh() { refresh = true; }
const char* Calendar_UnsupportedTimezone(uint8_t i) {
  return cache && i < CALENDAR_SOURCES && Calendar_State(i)==CAL_TIMEZONE ? cache[i].diagnostics.timezone : "";
}
uint32_t Calendar_Generation() { return generation; }
size_t Calendar_MemoryBytes() { return memoryBytes; }
CalendarSourceState Calendar_State(uint8_t i) {
  if (i >= CALENDAR_SOURCES || !Settings_ScreenEnabled(SCREEN_CALENDAR_I)) return CAL_OFF;
  const auto& c = Settings_CalendarSource(i);
  if (!c.enabled || !*c.url) return CAL_OFF;
  return cache ? cache[i].state : (memoryFailed ? CAL_MEMORY : CAL_WAIT);
}
time_t Calendar_Updated(uint8_t i) { return cache && i < CALENDAR_SOURCES ? cache[i].updated : 0; }
time_t Calendar_WindowStart() { return windowStart; }
time_t Calendar_WindowEnd() { return windowEnd; }
bool Calendar_HasData() {
  if (cache)
    for (int i = 0; i < CALENDAR_SOURCES; i++)
      if (Calendar_State(i) != CAL_OFF && cache[i].updated) return true;
  return false;
}
bool Calendar_Incomplete() {
  for (int i = 0; i < CALENDAR_SOURCES; i++) {
    auto s = Calendar_State(i);
    if (s != CAL_OFF && s != CAL_OK) return true;
  }
  return false;
}
size_t Calendar_Day(time_t day, const CalendarEvent** rows, size_t capacity) {
  if (!cache) return 0;
  time_t a, b;
  if (!dayRange(day, a, b)) return 0;
  size_t n = 0;
  for (int i = 0; i < CALENDAR_SOURCES; i++)
    if (Calendar_State(i) != CAL_OFF)
      for (size_t j = 0; j < cache[i].count; j++) {
        const auto& e = cache[i].events[j];
        if (e.start < b && (e.end > e.start ? e.end > a : e.start >= a)) {
          if (n < capacity) rows[n] = &e;
          n++;
        }
      }
  size_t used = n < capacity ? n : capacity;
  // The display has a fixed pointer array large enough for all three caches.
  for (size_t i = 1; i < used; i++) {
    const auto* e = rows[i];
    size_t j = i;
    while (j && ((e->allDay && !rows[j - 1]->allDay) ||
                 (e->allDay == rows[j - 1]->allDay && e->start < rows[j - 1]->start))) {
      rows[j] = rows[j - 1];
      j--;
    }
    rows[j] = e;
  }
  return n;
}
uint8_t Calendar_DayMask(time_t day) {
  if (!cache) return 0;
  time_t a, b;
  if (!dayRange(day, a, b)) return 0;
  uint8_t mask = 0;
  for (int i = 0; i < CALENDAR_SOURCES; i++)
    if (Calendar_State(i) != CAL_OFF)
      for (size_t j = 0; j < cache[i].count; j++) {
        const auto& e = cache[i].events[j];
        if (e.start < b && (e.end > e.start ? e.end > a : e.start >= a)) {
          mask |= 1 << i;
          break;
        }
      }
  return mask;
}
const char* Calendar_StateText(CalendarSourceState s) {
  switch (s) {
    case CAL_OFF:
      return "-";
    case CAL_WAIT:
      return T(S_LOADING);
    case CAL_OK:
      return "OK";
    case CAL_NETWORK:
      return T(S_CAL_NETWORK);
    case CAL_INVALID:
      return T(S_CAL_INVALID);
    case CAL_LIMIT:
      return T(S_CAL_LIMIT);
    case CAL_TIMEZONE:
      return T(S_CAL_TIMEZONE);
    case CAL_RULE:
      return T(S_CAL_RULE);
    case CAL_MEMORY:
      return T(S_CAL_MEMORY);
  }
  return "-";
}
void Calendar_Tick() {
  if (busy) return;
  if (!Settings_ScreenEnabled(SCREEN_CALENDAR_I) || !Settings_CalendarCount()) {
    if (cache) {
      release();
      generation++;
    }
    memoryFailed = false;
    revision = 0;
    return;
  }
  if (cache && revision != Settings_CalendarRevision()) syncSources(false);
  if (!Outside_TimeValid() || WiFi.status() != WL_CONNECTED) return;
  time_t now = time(nullptr);
  struct tm t{};
  localtime_r(&now, &t);
  time_t expected = now;
  t.tm_mon--;
  t.tm_mday = 1;
  t.tm_hour = t.tm_min = t.tm_sec = 0;
  t.tm_isdst = -1;
  expected = mktime(&t);
  bool changed = revision != Settings_CalendarRevision() || expected != windowStart;
  if (!cache) {
    if (memoryFailed && !refresh && millis() - memoryAttempt < 120000UL) return;
    memoryAttempt = millis();
    if (!allocate()) return;
  }
  if (changed) {
    bool moved = expected != windowStart;
    syncSources(moved);
    setWindow(now);
    cursor = 0;
  }
  if (refresh) {
    for (int i = 0; i < CALENDAR_SOURCES; i++) cache[i].tried = false;
    refresh = false;
  }
  for (int step = 0; step < CALENDAR_SOURCES; step++) {
    uint8_t i = (cursor + step) % CALENDAR_SOURCES;
    const auto& config = Settings_CalendarSource(i);
    if (!config.enabled || !*config.url) continue;
    auto& c = cache[i];
    unsigned long wait = (c.state == CAL_OK ? Settings_CalendarRefresh() * 60000UL : 120000UL);
    if (c.tried && millis() - c.attempted < wait) continue;
    cursor = (i + 1) % CALENDAR_SOURCES;
    c.tried = true;
    c.attempted = millis();
    busy = true;
    size_t length = 0, count = 0;
    bool tooLarge = false;
    bool fetched = Net_GetCalendar(config.url, feed, CALENDAR_MAX_FEED + 1, &length, &tooLarge);
    if (fetched) {
      auto result = CalendarIcs_Parse((char*)feed, length, windowStart, windowEnd, i, staging,
                                      CALENDAR_EVENTS_PER_SOURCE, count, scratch, parsePoll, &c.diagnostics);
      if (result == CAL_PARSE_OK) {
        qsort(staging, count, sizeof(CalendarEvent), order);
        memcpy(c.events, staging, count * sizeof(CalendarEvent));
        c.count = count;
        c.updated = now;
        c.state = CAL_OK;
      } else
        c.state = result == CAL_PARSE_LIMIT      ? CAL_LIMIT
                  : result == CAL_PARSE_TIMEZONE ? CAL_TIMEZONE
                  : result == CAL_PARSE_RULE     ? CAL_RULE
                                                 : CAL_INVALID;
    } else
      c.state = tooLarge ? CAL_LIMIT : CAL_NETWORK;
    busy = false;
    generation++;
    return;  // one source per loop, never parallel TLS sessions
  }
}
