#include "Layout.h"
#include <WiFi.h>
#include <esp_heap_caps.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "Calendar.h"
#include "ScreenChrome.h"
#include "Lang.h"
#include "ScreenCalendar.h"
#include "Settings.h"
#include "UI.h"

static time_t now = 1792846800;  // 2026-10-24 13:00 UTC
extern "C" time_t time(time_t* out) {
  if (out) *out = now;
  return now;
}
static std::string payload[3];
static int calls = 0;
static bool fail = false, large = false, clockValid = true;
bool Outside_TimeValid() { return clockValid; }
void Watchdog_Feed() {}
void Net_Poll() {}
bool Net_GetCalendar(const char* url, uint8_t* buf, size_t cap, size_t* len, bool* tooLarge) {
  calls++;
  *tooLarge = large;
  if (fail || large) return false;
  int index = url[strlen(url) - 1] - '0';
  assert(index >= 0 && index < 3);
  assert(payload[index].size() < cap);
  memcpy(buf, payload[index].c_str(), payload[index].size() + 1);
  *len = payload[index].size();
  return true;
}
Arduino_GFX canvas;
Arduino_GFX* gfx = &canvas;
void Outside_StatusText(char* out,size_t cap){snprintf(out,cap,"15:00  18 C");}
void UI_DrawStatusLine(int y) { Chrome_DrawStatus(y); }
void UI_TextCenteredIn(const char* text, int x, int width, int y, uint16_t color, uint8_t size) {
  gfx->setTextSize(size);
  gfx->setTextColor(color);
  gfx->setCursor(x + (width - Layout_TextW(text, size)) / 2, y);
  gfx->print(text);
}
void UI_TextCentered(const char* text, int y, uint16_t color, uint8_t size) {
  UI_TextCenteredIn(text, 0, 480, y, color, size);
}
int main() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  Settings_ClearAll(); Settings_SetClockFont(CLOCK_FONT_PIXEL);
  Settings_Begin();
  JsonDocument settings;
  deserializeJson(
      settings,
      R"({"screens":{"calendar":true},"calendars":[{"name":"Rodina","url":"https://example.test/0","enabled":true},{"name":"Praca","url":"https://example.test/1","enabled":true}]})");
  Settings_FromJson(settings.as<JsonObjectConst>());
  payload[0] =
      "BEGIN:VCALENDAR\nBEGIN:VEVENT\nUID:one\nDTSTART:20261024T100000Z\nDTEND:"
      "20261024T110000Z\nSUMMARY:Rodina\nEND:VEVENT\nEND:VCALENDAR\n";
  payload[1] =
      "BEGIN:VCALENDAR\nBEGIN:VEVENT\nUID:two\nDTSTART;VALUE=DATE:20261024\nDTEND;VALUE=DATE:"
      "20261026\nSUMMARY:Vikend\nEND:VEVENT\nEND:VCALENDAR\n";
  clockValid = false;
  Calendar_Tick();
  assert(calls == 0 && Calendar_MemoryBytes() == 0);
  clockValid = true;
  testWifiConnected = false;
  Calendar_Tick();
  assert(calls == 0);
  testWifiConnected = true;
  testFailPsram = true;
  Calendar_Tick();
  assert(Calendar_State(0) == CAL_MEMORY && Calendar_MemoryBytes() == 0);
  testFailPsram = false;
  Calendar_Refresh();
  Calendar_Tick();
  assert(calls == 1 && Calendar_State(0) == CAL_OK && Calendar_State(1) == CAL_WAIT);
  assert(Calendar_Incomplete());
  assert(Calendar_MemoryBytes() < 768 * 1024);
  std::cout << "Calendar PSRAM budget: " << Calendar_MemoryBytes() << " bytes\n";
  Calendar_Tick();
  assert(calls == 2 && Calendar_State(1) == CAL_OK && !Calendar_Incomplete());
  assert(Calendar_DayMask(now) == 3);
  const CalendarEvent* rows[CALENDAR_SOURCES * CALENDAR_EVENTS_PER_SOURCE];
  assert(Calendar_Day(now, rows, CALENDAR_SOURCES * CALENDAR_EVENTS_PER_SOURCE) == 2);
  assert(rows[0]->allDay);  // all-day row before timed events
  Settings_SetLanguage(LANG_SK);
  ScreenCalendar_SelectDay(now);
  ScreenCalendar_Enter();
  Settings_SetCalendarView(0);
  ScreenCalendar_Draw();
  assert(canvas.circles == 4);  // two agenda dots + two legend dots
  if (const char* dir = getenv("CALENDAR_SCREENSHOTS")) {
    std::string path = std::string(dir) + "/calendar-day-multiple.svg";
    canvas.snapshot(path.c_str());
  }
  Settings_SetCalendarView(1);
  ScreenCalendar_Draw();
  assert(canvas.circles > 2);
  if (const char* dir = getenv("CALENDAR_SCREENSHOTS")) {
    std::string path = std::string(dir) + "/calendar-month-multiple.svg";
    canvas.snapshot(path.c_str());
  }
  // October 24 lies in row 3, column 5 in a Monday-first October 2026 grid.
  assert(ScreenCalendar_HandleTap(340, 292));
  assert(Settings_CalendarView() == 0);
  char range[32];
  ScreenCalendar_RangeText(range, sizeof(range));
  assert(!strcmp(range, "24.10.2026"));
  ScreenCalendar_ChangeRange(1);
  ScreenCalendar_RangeText(range, sizeof(range));
  assert(!strcmp(range, "25.10.2026"));
  ScreenCalendar_SelectDay(now);
  int before = calls;
  Calendar_Tick();
  assert(calls == before);
  fail = true;
  Calendar_Refresh();
  Calendar_Tick();
  assert(Calendar_State(0) == CAL_NETWORK);
  assert(Calendar_Day(now, rows, CALENDAR_SOURCES * CALENDAR_EVENTS_PER_SOURCE) == 2 &&
         Calendar_Incomplete());
  fail = false;
  payload[1] = "<html>login required</html>";
  Calendar_Tick();
  assert(Calendar_State(1) == CAL_INVALID);
  assert(Calendar_Day(now, rows, CALENDAR_SOURCES * CALENDAR_EVENTS_PER_SOURCE) == 2);
  large = true;
  Calendar_Refresh();
  Calendar_Tick();
  assert(Calendar_State(0) == CAL_LIMIT);
  large = false;
  large = false;
  fail = false;
  settings.clear();
  deserializeJson(settings, R"({"calendars":[{}, {"enabled":false}]})");
  Settings_FromJson(settings.as<JsonObjectConst>());
  Calendar_Refresh();
  Calendar_Tick();
  assert(Settings_CalendarCount() == 1);
  Settings_SetCalendarView(0);
  ScreenCalendar_Draw();
  assert(canvas.circles == 0);
  if (const char* dir = getenv("CALENDAR_SCREENSHOTS")) {
    std::string path = std::string(dir) + "/calendar-day-single.svg";
    canvas.snapshot(path.c_str());
  }
  Settings_SetCalendarView(1);
  ScreenCalendar_Draw();
  assert(canvas.circles == 0);
  if (const char* dir = getenv("CALENDAR_SCREENSHOTS")) {
    std::string path = std::string(dir) + "/calendar-month-single.svg";
    canvas.snapshot(path.c_str());
  }
  Settings_SetClockFont(CLOCK_FONT_INTER);
  if (const char* dir = getenv("CALENDAR_SCREENSHOTS")) {
    for (uint8_t view=0;view<2;++view) {
      Settings_SetCalendarView(view); ScreenCalendar_Draw();
      canvas.snapshot((std::string(dir)+"/calendar-smooth-"+std::to_string(view)+".svg").c_str());
    }
  }
  Settings_SetClockFont(CLOCK_FONT_PIXEL);
  // A failed zone is diagnosed per source without replacing its valid cache.
  payload[0]="BEGIN:VCALENDAR\r\nBEGIN:VEVENT\r\nDTSTART;TZID=Unknown/PrivateZone:20261024T100000\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n";
  Calendar_Refresh();Calendar_Tick();
  assert(Calendar_State(0)==CAL_TIMEZONE);
  assert(!strcmp(Calendar_UnsupportedTimezone(0),"Unknown/PrivateZone"));
  assert(Calendar_Day(now,rows,CALENDAR_SOURCES*CALENDAR_EVENTS_PER_SOURCE)>0);
  assert(!*Calendar_UnsupportedTimezone(1));
  // A new subscription must not display the previous URL's private events.
  settings.clear();
  deserializeJson(settings, R"({"calendars":[{"clearUrl":true},{"enabled":false}]})");
  Settings_FromJson(settings.as<JsonObjectConst>());
  Calendar_Tick();
  assert(Calendar_MemoryBytes() == 0 && Calendar_DayMask(now) == 0);
  std::cout << "Calendar scheduler, PSRAM allocation, retry, stale-cache and disabled-source tests "
               "passed\n";
}
