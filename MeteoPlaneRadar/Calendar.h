#pragma once
#include "CalendarIcs.h"
#include "Config.h"

enum CalendarSourceState : uint8_t {
  CAL_OFF,
  CAL_WAIT,
  CAL_OK,
  CAL_NETWORK,
  CAL_INVALID,
  CAL_LIMIT,
  CAL_TIMEZONE,
  CAL_RULE,
  CAL_MEMORY
};
void Calendar_Tick();
void Calendar_Refresh();  // queue only; downloads run from loop
uint32_t Calendar_Generation();
size_t Calendar_MemoryBytes();
CalendarSourceState Calendar_State(uint8_t source);
time_t Calendar_Updated(uint8_t source);
time_t Calendar_WindowStart();
time_t Calendar_WindowEnd();
bool Calendar_HasData();
bool Calendar_Incomplete();
// Sorted events overlapping a local calendar day, including multi-day events.
size_t Calendar_Day(time_t day, const CalendarEvent** rows, size_t capacity);
uint8_t Calendar_DayMask(time_t day);
const char* Calendar_StateText(CalendarSourceState state);

const char* Calendar_UnsupportedTimezone(uint8_t source);
