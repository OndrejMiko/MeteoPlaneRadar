#pragma once
#include <stddef.h>
#include <stdint.h>
#include <time.h>

// Bounded, display-ready instances. No String/JSON document or framebuffer.
struct CalendarEvent {
  time_t start, end;
  uint64_t uidHash;
  char title[64], location[48];  // ASCII for the built-in display font
  uint8_t source;
  bool allDay;
};
enum CalendarParseResult {
  CAL_PARSE_OK,
  CAL_PARSE_INVALID,
  CAL_PARSE_LIMIT,
  CAL_PARSE_TIMEZONE,
  CAL_PARSE_RULE
};
// Only the failing TZID is exposed, never the feed URL or event contents.
struct CalendarParseDiagnostics {
  char timezone[128];
};
// Scratch is caller-owned (PSRAM on the device), never on the loop stack.
size_t CalendarIcs_WorkspaceSize();
CalendarParseResult CalendarIcs_Parse(const char* data, size_t length, time_t from, time_t until,
                                      uint8_t source, CalendarEvent* out, size_t capacity,
                                      size_t& count, void* workspace, void (*poll)() = nullptr,
                                      CalendarParseDiagnostics* diagnostics = nullptr);
void Calendar_Ascii(const char* in, char* out, size_t cap, bool icsEscapes = true,
                    bool newlines = false);
