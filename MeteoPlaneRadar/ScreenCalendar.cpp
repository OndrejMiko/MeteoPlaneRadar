#include "ScreenCalendar.h"

#include <stdio.h>
#include <string.h>

#include "Calendar.h"
#include "Lang.h"
#include "Layout.h"
#include "Outside.h"
#include "Settings.h"
#include "UI.h"

namespace {
constexpr int GRID_X = 65, GRID_Y = 205, CELL_W = 50, CELL_H = 29;
constexpr int AGENDA_Y = 202, ROW_H = 57, PER_PAGE = 3;
// A pointer per cached event, not another copy of the events or a framebuffer.
const CalendarEvent* rows[CALENDAR_SOURCES * CALENDAR_EVENTS_PER_SOURCE];
time_t selected = 0, lastToday = 0;
size_t page = 0;
uint32_t seen = 0;
int seenMinute = -1, seenLang = -1, seenView = -1;

// Every date operation goes through mktime with tm_isdst=-1, including 23/25h days.
time_t midnight(time_t value) {
  struct tm t{};
  localtime_r(&value, &t);
  t.tm_hour = t.tm_min = t.tm_sec = 0;
  t.tm_isdst = -1;
  return mktime(&t);
}
time_t dayShift(time_t value, int n) {
  struct tm t{};
  localtime_r(&value, &t);
  t.tm_mday += n;
  t.tm_isdst = -1;
  return mktime(&t);
}
bool sameDate(time_t a, time_t b) { return midnight(a) == midnight(b); }
void text(const char* s, int x, int y, uint16_t color, uint8_t size) {
  gfx->setTextColor(color);
  gfx->setTextSize(size);
  gfx->setCursor(x, y);
  gfx->print(s);
}
void fit(const char* s, char* out, size_t cap, int pixels, uint8_t size) {
  size_t n = strlen(s), max = pixels / LY_CHAR_W(size);
  if (n > max) n = max;
  if (n >= cap) n = cap - 1;
  memcpy(out, s, n);
  out[n] = 0;
  if (strlen(s) > n && n >= 3) memcpy(out + n - 3, "...", 3);
}
void button(int x, int y, int w, int h, const char* label, bool active) {
  gfx->fillRoundRect(x, y, w, h, 14, active ? C_CYAN : C_DKGRAY);
  UI_TextCenteredIn(label, x, w, y + (h - LY_CHAR_H(2)) / 2, active ? C_BLACK : C_WHITE, 2);
}
bool inWindow(time_t day) {
  return !Calendar_WindowStart() || (day >= Calendar_WindowStart() && day < Calendar_WindowEnd());
}
void note(const char* s, uint16_t color = C_GRAY) { UI_TextCentered(s, 385, color, 1); }
void drawMonth() {
  struct tm t{};
  localtime_r(&selected, &t);
  t.tm_mday = 1;
  t.tm_isdst = -1;
  time_t first = mktime(&t);
  int offset = (t.tm_wday + 6) % 7;
  for (int col = 0; col < 7; col++)
    UI_TextCenteredIn(Lang_WeekdayShort((col + 1) % 7), GRID_X + col * CELL_W, CELL_W, 183, C_GRAY,
                      1);
  time_t grid = dayShift(first, -offset);
  bool multiple = Settings_CalendarCount() > 1;
  for (int i = 0; i < 42; i++) {
    time_t day = dayShift(grid, i);
    struct tm d{};
    localtime_r(&day, &d);
    int x = GRID_X + (i % 7) * CELL_W, y = GRID_Y + (i / 7) * CELL_H;
    bool current = d.tm_mon == t.tm_mon, loaded = inWindow(day);
    uint8_t mask = loaded ? Calendar_DayMask(day) : 0;
    if (sameDate(day, selected) && current) gfx->fillRoundRect(x + 5, y - 4, 40, 27, 6, C_DKGRAY);
    if (sameDate(day, time(nullptr))) gfx->drawRoundRect(x + 5, y - 4, 40, 27, 6, C_CYAN);
    char b[8];
    snprintf(b, sizeof(b), "%d", d.tm_mday);
    UI_TextCenteredIn(b, x, CELL_W, y, current ? (mask && !multiple ? C_CYAN : C_WHITE) : C_GRAY,
                      2);
    // A single subscription uses text emphasis, never a dot or a source legend.
    if (multiple && mask) {
      int n = 0;
      for (int k = 0; k < CALENDAR_SOURCES; k++)
        if (mask & (1 << k)) n++;
      int dot = x + CELL_W / 2 - (n - 1) * 5;
      for (int k = 0; k < CALENDAR_SOURCES; k++)
        if (mask & (1 << k)) {
          gfx->fillCircle(dot, y + 20, 2, Settings_CalendarSource(k).color);
          dot += 10;
        }
    }
  }
  if (Calendar_Incomplete())
    note(Calendar_HasData() ? T(S_CAL_STALE) : T(S_LOADING), C_YELLOW);
  else
    note(T(S_CAL_PICK));
}
void drawDay() {
  if (!Outside_TimeValid()) {
    UI_TextCentered(T(S_CAL_TIME_WAIT), 248, C_GRAY, 2);
    return;
  }
  if (!Settings_CalendarCount()) {
    UI_TextCentered(T(S_CAL_SETUP), 240, C_CYAN, 2);
    note(T(S_WEB_HINT));
    return;
  }
  if (!Calendar_HasData()) {
    UI_TextCentered(T(S_LOADING), 238, C_GRAY, 2);
    for (int i = 0; i < CALENDAR_SOURCES; i++)
      if (Calendar_State(i) > CAL_OK) {
        note(Calendar_StateText(Calendar_State(i)), C_YELLOW);
        break;
      }
    return;
  }
  if (!inWindow(selected)) {
    UI_TextCentered(T(S_CAL_OUTSIDE), 240, C_GRAY, 2);
    return;
  }
  size_t total = Calendar_Day(selected, rows, sizeof(rows) / sizeof(*rows));
  size_t pages = (total + PER_PAGE - 1) / PER_PAGE;
  if (page >= pages) page = 0;
  if (!total) {
    UI_TextCentered(T(S_CAL_EMPTY), 244, C_GRAY, 2);
    if (Calendar_Incomplete()) note(T(S_CAL_STALE), C_YELLOW);
    return;
  }
  bool multiple = Settings_CalendarCount() > 1;
  for (int i = 0; i < PER_PAGE; i++) {
    size_t idx = page * PER_PAGE + i;
    if (idx >= total) break;
    const auto& e = *rows[idx];
    int y = AGENDA_Y + i * ROW_H;
    char start[16], end[16], title[40], meta[64];
    struct tm a{}, b{};
    localtime_r(&e.start, &a);
    localtime_r(&e.end, &b);
    if (e.allDay) {
      snprintf(start, sizeof(start), "%s", T(S_CAL_ALLDAY));
      end[0] = 0;
    } else {
      snprintf(start, sizeof(start), "%02d:%02d", a.tm_hour, a.tm_min);
      snprintf(end, sizeof(end), "%02d:%02d", b.tm_hour, b.tm_min);
    }
    // Content is kept in the central chord, including a three-line page.
    text(start, 68, y, C_WHITE, e.allDay ? 1 : 2);
    text(end, 68, y + 22, C_GRAY, 1);
    fit(e.title, title, sizeof(title), 260, 2);
    text(title, 146, y, C_WHITE, 2);
    if (multiple) {
      Calendar_Ascii(Settings_CalendarSource(e.source).name, meta, sizeof(meta));
      char shortName[24];
      fit(meta, shortName, sizeof(shortName), 232, 1);
      gfx->fillCircle(149, y + 27, 3, Settings_CalendarSource(e.source).color);
      text(shortName, 160, y + 23, C_GRAY, 1);
    } else if (*e.location) {
      fit(e.location, meta, sizeof(meta), 260, 1);
      text(meta, 146, y + 23, C_GRAY, 1);
    }
    if (i < PER_PAGE - 1 && idx + 1 < total) gfx->drawFastHLine(68, y + 47, 344, C_DKGRAY);
  }
  if (pages > 1) {
    char b[32];
    snprintf(b, sizeof(b), "%u/%u  %s", (unsigned)(page + 1), (unsigned)pages, T(S_CAL_MORE));
    note(b);
  } else if (Calendar_Incomplete())
    note(T(S_CAL_STALE), C_YELLOW);
}
}  // namespace
void ScreenCalendar_SelectDay(time_t day) {
  selected = midnight(day);
  page = 0;
}
void ScreenCalendar_Enter() {
  if (!selected && Outside_TimeValid()) selected = midnight(time(nullptr));
  seen = 0;
}
bool ScreenCalendar_Tick() {
  time_t now = time(nullptr);
  time_t today = Outside_TimeValid() ? midnight(now) : 0;
  if (today != lastToday) {
    if (!selected || selected == lastToday) selected = today;
    lastToday = today;
    page = 0;
    seen = 0;
  }
  uint32_t gen = Calendar_Generation();
  int minute = now / 60;
  bool dirty = gen != seen || seenMinute != minute || seenLang != Lang_Get() ||
               seenView != Settings_CalendarView();
  seen = gen;
  seenMinute = minute;
  seenLang = Lang_Get();
  seenView = Settings_CalendarView();
  return dirty;
}
void ScreenCalendar_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();
  Layout_ReserveBand(LY_STATUS, 16);
  UI_DrawStatusLine(LY_STATUS);
  UI_TextCentered(T(S_CALENDAR), 78, C_CYAN, 1);
  char b[48];
  struct tm date{};
  time_t value = selected ? selected : time(nullptr);
  localtime_r(&value, &date);
  if (Settings_CalendarView())
    snprintf(b, sizeof(b), "%s %d", Lang_MonthLabel(date.tm_mon), date.tm_year + 1900);
  else
    snprintf(b, sizeof(b), "%s %d.%d.", Lang_WeekdayShort(date.tm_wday), date.tm_mday,
             date.tm_mon + 1);
  if (!Outside_TimeValid()) snprintf(b, sizeof(b), "--.--.");
  UI_TextCentered(b, 99, C_WHITE, 2);
  // Labels stay inside generous touch regions, with screen long-press unchanged.
  text("<", 48, 98, C_GRAY, 3);
  text(">", 414, 98, C_GRAY, 3);
  button(132, 138, 108, 34, T(S_CAL_DAY), !Settings_CalendarView());
  button(244, 138, 108, 34, T(S_CAL_MONTH), Settings_CalendarView());
  if (Settings_CalendarView() && Settings_CalendarCount() && Outside_TimeValid() &&
      Calendar_HasData())
    drawMonth();
  else
    drawDay();
  button(174, 407, 132, 30, T(S_CAL_TODAY), false);
  // Source names and colours are meaningful only with multiple subscriptions.
  if (Settings_CalendarCount() > 1) {
    int x = 240 - Settings_CalendarCount() * 68 / 2;
    for (int i = 0; i < CALENDAR_SOURCES; i++) {
      const auto& c = Settings_CalendarSource(i);
      if (c.enabled && *c.url) {
        char name[32], shortName[8];
        Calendar_Ascii(c.name, name, sizeof(name));
        fit(name, shortName, sizeof(shortName), 42, 1);
        gfx->fillCircle(x + 3, 452, 3, c.color);
        text(shortName, x + 12, 448, C_GRAY, 1);
        x += 68;
      }
    }
  }
}
bool ScreenCalendar_HandleTap(int x, int y) {
  if (y >= 82 && y <= 127) {
    if (x >= 25 && x <= 102) {
      ScreenCalendar_ChangeRange(-1);
      return true;
    }
    if (x >= 378 && x <= 455) {
      ScreenCalendar_ChangeRange(1);
      return true;
    }
  }
  if (y >= 132 && y <= 177 && x >= 122 && x <= 362) {
    Settings_SetCalendarView(x < 242 ? 0 : 1);
    page = 0;
    return true;
  }
  if (y >= 400 && y <= 445 && x >= 164 && x <= 316) {
    selected = midnight(time(nullptr));
    page = 0;
    return true;
  }
  if (Settings_CalendarView() && x >= GRID_X && x < GRID_X + 7 * CELL_W && y >= GRID_Y - 4 &&
      y < GRID_Y + 6 * CELL_H - 4) {
    struct tm d{};
    localtime_r(&selected, &d);
    d.tm_mday = 1;
    d.tm_isdst = -1;
    time_t first = mktime(&d);
    int offset = (d.tm_wday + 6) % 7;
    int cell = ((y - GRID_Y + 4) / CELL_H) * 7 + (x - GRID_X) / CELL_W;
    time_t target = dayShift(first, cell - offset);
    if (inWindow(target)) {
      selected = target;
      Settings_SetCalendarView(0);
      page = 0;
    }
    return true;
  }
  if (!Settings_CalendarView() && y >= AGENDA_Y && y < 400) {
    size_t total = Calendar_Day(selected, rows, sizeof(rows) / sizeof(*rows));
    if (total > PER_PAGE) {
      page = (page + 1) % ((total + PER_PAGE - 1) / PER_PAGE);
      return true;
    }
  }
  return false;
}
void ScreenCalendar_ChangeRange(int dir) {
  if (!selected) selected = midnight(time(nullptr));
  time_t next;
  if (Settings_CalendarView()) {
    struct tm d{};
    localtime_r(&selected, &d);
    d.tm_mday = 1;
    d.tm_mon += dir < 0 ? -1 : 1;
    d.tm_isdst = -1;
    next = mktime(&d);
  } else
    next = dayShift(selected, dir < 0 ? -1 : 1);
  if (inWindow(next)) {
    selected = next;
    page = 0;
  }
}
void ScreenCalendar_RangeText(char* out, size_t cap) {
  struct tm d{};
  time_t value = selected ? selected : time(nullptr);
  localtime_r(&value, &d);
  snprintf(out, cap, "%d.%d.%d", d.tm_mday, d.tm_mon + 1, d.tm_year + 1900);
}
