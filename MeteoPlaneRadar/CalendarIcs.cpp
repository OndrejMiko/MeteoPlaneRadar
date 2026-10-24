#include "CalendarIcs.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
constexpr int LINE = 1024, EXCEPTIONS = 256;
struct Date {
  int y = 0, m = 0, d = 0, h = 0, min = 0, s = 0;
  bool all = false, utc = false;
};
struct Stamp {
  Date wall;
  time_t epoch = 0;
  char zone[64]{};
};
struct Raw {
  Stamp start, end, rid;
  bool hasStart = false, hasEnd = false, hasRid = false, cancelled = false;
  uint64_t uid = 0;
  char title[64]{}, location[48]{}, rule[256]{};
  time_t excluded[64]{};
  int excludedN = 0;
  time_t extra[64]{};
  int extraN = 0;
  int duration = 0;
};
struct Exception {
  uint64_t uid;
  time_t rid;
};
struct Work {
  Raw raw;
  Exception exceptions[EXCEPTIONS];
  size_t exceptionN;
  char line[LINE];
  unsigned long steps;
};
struct Rule {
  int freq = 0, interval = 1, count = 0, wkst = 1;
  time_t until = 0;
  int months[12]{}, monthN = 0, mdays[31]{}, mdayN = 0;
  int days[7]{}, ordinals[7]{}, dayN = 0;
};
int dim(int y, int m) {
  static const int n[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return n[m - 1] + (m == 2 && y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
}
// Gregorian days since 1970; independent of the device's time zone.
long civil(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yo = y - era * 400;
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  return era * 146097L + yo * 365 + yo / 4 - yo / 100 + doy - 719468;
}
Date dateFrom(long days) {
  time_t t = (time_t)days * 86400;
  struct tm tm{};
  gmtime_r(&t, &tm);
  Date d;
  d.y = tm.tm_year + 1900;
  d.m = tm.tm_mon + 1;
  d.d = tm.tm_mday;
  return d;
}
int weekday(long day) {
  int w = (day + 4) % 7;
  return w < 0 ? w + 7 : w;
}
int lastSunday(int y, int m) {
  const int n = dim(y, m);
  return n - weekday(civil(y, m, n));
}
bool euZone(const char* z) {
  const char* names[] = {"Europe/Bratislava",
                         "Europe/Prague",
                         "Europe/Berlin",
                         "Europe/Vienna",
                         "Europe/Paris",
                         "Europe/Rome",
                         "Europe/Warsaw",
                         "Europe/Budapest",
                         "Europe/Amsterdam",
                         "Europe/Brussels",
                         "Europe/Madrid",
                         "Europe/Zurich",
                         "Central Europe Standard Time",
                         "W. Europe Standard Time",
                         "CET",
                         "CEST"};
  for (auto n : names)
    if (!strcmp(z, n)) return true;
  return false;
}
bool epoch(const Date& d, const char* zone, time_t& out) {
  if (d.y < 1900 || d.y > 2100 || d.m < 1 || d.m > 12 || d.d < 1 || d.d > dim(d.y, d.m) ||
      d.h > 23 || d.min > 59 || d.s > 59)
    return false;
  const time_t naive = (time_t)civil(d.y, d.m, d.d) * 86400 + d.h * 3600 + d.min * 60 + d.s;
  if (d.utc || !strcmp(zone, "UTC") || !strcmp(zone, "Etc/UTC") || !strcmp(zone, "GMT")) {
    out = naive;
    return true;
  }
  // DATE values are calendar dates in the display's local zone, never UTC.
  if (!*zone || d.all) {
    struct tm t{};
    t.tm_year = d.y - 1900;
    t.tm_mon = d.m - 1;
    t.tm_mday = d.d;
    t.tm_hour = d.h;
    t.tm_min = d.min;
    t.tm_sec = d.s;
    t.tm_isdst = -1;
    out = mktime(&t);
    return out != (time_t)-1;
  }
  if (euZone(zone)) {
    const bool dst =
        (d.m > 3 && d.m < 10) ||
        (d.m == 3 && (d.d > lastSunday(d.y, 3) || (d.d == lastSunday(d.y, 3) && d.h >= 3))) ||
        (d.m == 10 && (d.d < lastSunday(d.y, 10) || (d.d == lastSunday(d.y, 10) && d.h < 3)));
    out = naive - (dst ? 7200 : 3600);
    return true;
  }
  return false;
}
int digits(const char* s, int n) {
  int v = 0;
  for (int i = 0; i < n; i++) {
    if (!isdigit((unsigned char)s[i])) return -1;
    v = v * 10 + s[i] - '0';
  }
  return v;
}
CalendarParseResult stamp(const char* s, const char* zone, Stamp& out) {
  size_t n = strlen(s);
  if (n != 8 && n != 15 && n != 16) return CAL_PARSE_INVALID;
  Date d;
  d.y = digits(s, 4);
  d.m = digits(s + 4, 2);
  d.d = digits(s + 6, 2);
  d.all = n == 8;
  if (!d.all) {
    if (s[8] != 'T' || (n == 16 && s[15] != 'Z')) return CAL_PARSE_INVALID;
    d.h = digits(s + 9, 2);
    d.min = digits(s + 11, 2);
    d.s = digits(s + 13, 2);
    d.utc = n == 16;
  }
  if (d.h < 0 || d.min < 0 || d.s < 0) return CAL_PARSE_INVALID;
  out.wall = d;
  snprintf(out.zone, sizeof(out.zone), "%s", zone);
  if (!epoch(d, zone, out.epoch))
    return (*zone && !euZone(zone) && !d.utc && !d.all && strcmp(zone, "UTC") &&
            strcmp(zone, "Etc/UTC") && strcmp(zone, "GMT"))
               ? CAL_PARSE_TIMEZONE
               : CAL_PARSE_INVALID;
  return CAL_PARSE_OK;
}
uint64_t hash(const char* s) {
  uint64_t h = 14695981039346656037ULL;
  for (; *s; s++) h = (h ^ (uint8_t)*s) * 1099511628211ULL;
  return h;
}
bool integer(const char* s, int& out, int lo, int hi) {
  char* end;
  long n = strtol(s, &end, 10);
  if (!*s || *end || n < lo || n > hi) return false;
  out = n;
  return true;
}
bool listInts(char* s, int* out, int& n, int max, int lo, int hi) {
  char* save = nullptr;
  for (char* t = strtok_r(s, ",", &save); t; t = strtok_r(nullptr, ",", &save)) {
    if (n == max || !integer(t, out[n], lo, hi) || out[n] == 0) return false;
    n++;
  }
  return n > 0;
}
int weekDay(const char* s) {
  const char* names[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
  for (int i = 0; i < 7; i++)
    if (!strcmp(s, names[i])) return i;
  return -1;
}
bool ruleParse(const Raw& raw, Rule& r) {
  char text[256];
  snprintf(text, sizeof(text), "%s", raw.rule);
  char* save = nullptr;
  for (char* t = strtok_r(text, ";", &save); t; t = strtok_r(nullptr, ";", &save)) {
    char* v = strchr(t, '=');
    if (!v) return false;
    *v++ = 0;
    if (!strcmp(t, "FREQ")) {
      if (!strcmp(v, "DAILY"))
        r.freq = 1;
      else if (!strcmp(v, "WEEKLY"))
        r.freq = 2;
      else if (!strcmp(v, "MONTHLY"))
        r.freq = 3;
      else if (!strcmp(v, "YEARLY"))
        r.freq = 4;
      else
        return false;
    } else if (!strcmp(t, "INTERVAL")) {
      if (!integer(v, r.interval, 1, 366)) return false;
    } else if (!strcmp(t, "COUNT")) {
      if (!integer(v, r.count, 1, 100000)) return false;
    } else if (!strcmp(t, "UNTIL")) {
      Stamp u;
      if (stamp(v, raw.start.zone, u) != CAL_PARSE_OK) return false;
      r.until = u.epoch;
      if (u.wall.all) r.until += 86399;
    } else if (!strcmp(t, "WKST")) {
      r.wkst = weekDay(v);
      if (r.wkst < 0) return false;
    } else if (!strcmp(t, "BYMONTH")) {
      if (!listInts(v, r.months, r.monthN, 12, 1, 12)) return false;
    } else if (!strcmp(t, "BYMONTHDAY")) {
      if (!listInts(v, r.mdays, r.mdayN, 31, -31, 31)) return false;
    } else if (!strcmp(t, "BYDAY")) {
      char* ds = nullptr;
      for (char* d = strtok_r(v, ",", &ds); d; d = strtok_r(nullptr, ",", &ds)) {
        if (r.dayN == 7 || strlen(d) < 2) return false;
        size_t len = strlen(d);
        r.days[r.dayN] = weekDay(d + len - 2);
        if (r.days[r.dayN] < 0) return false;
        d[len - 2] = 0;
        if (*d && (!integer(d, r.ordinals[r.dayN], -5, 5) || !r.ordinals[r.dayN])) return false;
        r.dayN++;
      }
    } else
      return false;  // Never silently misinterpret a more complex rule.
  }
  if (!r.freq || (r.count && r.until)) return false;
  if (r.freq == 4 && (r.dayN || (!r.monthN && r.mdayN))) return false;
  for (int i = 0; i < r.dayN; i++)
    if (r.ordinals[i] && r.freq != 3) return false;
  return true;
}
bool member(const int* a, int n, int v) {
  for (int i = 0; i < n; i++)
    if (a[i] == v) return true;
  return false;
}
bool matches(const Date& d, long day, const Raw& raw, const Rule& r) {
  const auto& start = raw.start.wall;
  const long first = civil(start.y, start.m, start.d), delta = day - first;
  if (r.freq == 1 && delta % r.interval) return false;
  if (r.freq == 2) {
    long a = first - (weekday(first) - r.wkst + 7) % 7, b = day - (weekday(day) - r.wkst + 7) % 7;
    if (((b - a) / 7) % r.interval) return false;
    if (!r.dayN && weekday(day) != weekday(first)) return false;
  }
  if (r.freq == 3 && ((d.y - start.y) * 12 + d.m - start.m) % r.interval) return false;
  if (r.freq == 4 && (d.y - start.y) % r.interval) return false;
  if (r.monthN && !member(r.months, r.monthN, d.m)) return false;
  if (r.freq == 4 && !r.monthN && d.m != start.m) return false;
  if (r.mdayN) {
    bool yes = false;
    for (int i = 0; i < r.mdayN; i++) {
      int want = r.mdays[i] > 0 ? r.mdays[i] : dim(d.y, d.m) + r.mdays[i] + 1;
      if (d.d == want) yes = true;
    }
    if (!yes) return false;
  }
  if (r.dayN) {
    bool yes = false;
    for (int i = 0; i < r.dayN; i++)
      if (weekday(day) == r.days[i]) {
        int ord = r.ordinals[i];
        if (!ord || (ord > 0 && (d.d - 1) / 7 + 1 == ord) ||
            (ord < 0 && -((dim(d.y, d.m) - d.d) / 7 + 1) == ord))
          yes = true;
      }
    if (!yes) return false;
  }
  if ((r.freq == 3 || r.freq == 4) && !r.mdayN && !r.dayN && d.d != start.d) return false;
  return true;
}
// Read and unfold one logical content line. Long unused descriptions/alarms are
// skipped safely; important overlong properties are rejected by the caller.
bool line(const char* data, size_t len, size_t& pos, char* out, bool& over) {
  if (pos >= len) return false;
  size_t n = 0;
  over = false;
  do {
    while (pos < len && data[pos] != '\r' && data[pos] != '\n') {
      if (n < LINE - 1)
        out[n++] = data[pos];
      else
        over = true;
      pos++;
    }
    if (pos < len && data[pos] == '\r') pos++;
    if (pos < len && data[pos] == '\n') pos++;
    if (pos < len && (data[pos] == ' ' || data[pos] == '\t')) {
      pos++;
    } else
      break;
  } while (pos < len);
  out[n] = 0;
  return true;
}
bool zoneParam(char* prop, char* zone, size_t cap) {
  zone[0] = 0;
  char* t = strstr(prop, ";TZID=");
  if (!t) return true;
  t += 6;
  if (*t == '"') {
    t++;
    char* e = strchr(t, '"');
    if (!e) return false;
    *e = 0;
  } else {
    char* e = strchr(t, ';');
    if (e) *e = 0;
  }
  if (strlen(t) >= cap) return false;
  strcpy(zone, t);
  return true;
}
CalendarParseResult property(Raw& r, char* key, char* val) {
  char zone[64];
  if (strstr(key, ";RANGE=")) return CAL_PARSE_RULE;
  if (!zoneParam(key, zone, sizeof(zone))) return CAL_PARSE_TIMEZONE;
  char* sem = strchr(key, ';');
  if (sem) *sem = 0;
  if (!strcmp(key, "UID"))
    r.uid = hash(val);
  else if (!strcmp(key, "SUMMARY"))
    Calendar_Ascii(val, r.title, sizeof(r.title));
  else if (!strcmp(key, "LOCATION"))
    Calendar_Ascii(val, r.location, sizeof(r.location));
  else if (!strcmp(key, "STATUS"))
    r.cancelled = !strcmp(val, "CANCELLED");
  else if (!strcmp(key, "DTSTART")) {
    auto e = stamp(val, zone, r.start);
    if (e != CAL_PARSE_OK) return e;
    r.hasStart = true;
  } else if (!strcmp(key, "DTEND")) {
    auto e = stamp(val, zone, r.end);
    if (e != CAL_PARSE_OK) return e;
    r.hasEnd = true;
  } else if (!strcmp(key, "RECURRENCE-ID")) {
    if (strstr(sem ? sem + 1 : "", "RANGE=")) return CAL_PARSE_RULE;
    auto e = stamp(val, zone, r.rid);
    if (e != CAL_PARSE_OK) return e;
    r.hasRid = true;
  } else if (!strcmp(key, "EXRULE"))
    return CAL_PARSE_RULE;
  else if (!strcmp(key, "RRULE")) {
    if (strlen(val) >= sizeof(r.rule)) return CAL_PARSE_RULE;
    strcpy(r.rule, val);
  } else if (!strcmp(key, "EXDATE") || !strcmp(key, "RDATE")) {
    bool ex = !strcmp(key, "EXDATE");
    int& n = ex ? r.excludedN : r.extraN;
    time_t* a = ex ? r.excluded : r.extra;
    char* save = nullptr;
    for (char* t = strtok_r(val, ",", &save); t; t = strtok_r(nullptr, ",", &save)) {
      if (n == 64) return CAL_PARSE_LIMIT;
      Stamp s;
      auto e = stamp(t, zone, s);
      if (e != CAL_PARSE_OK) return e;
      a[n++] = s.epoch;
    }
  } else if (!strcmp(key, "DURATION")) {
    bool time = false;
    int total = 0;
    const char* v = val;
    if (*v++ != 'P') return CAL_PARSE_INVALID;
    while (*v) {
      if (*v == 'T') {
        time = true;
        v++;
        continue;
      }
      char* e;
      long n = strtol(v, &e, 10);
      if (e == v || n < 0 || n > 366) return CAL_PARSE_INVALID;
      int mul = 0;
      switch (*e) {
        case 'W':
          if (!time) mul = 604800;
          break;
        case 'D':
          if (!time) mul = 86400;
          break;
        case 'H':
          if (time) mul = 3600;
          break;
        case 'M':
          if (time) mul = 60;
          break;
        case 'S':
          if (time) mul = 1;
          break;
      }
      if (!mul || n > (INT_MAX - total) / mul) return CAL_PARSE_INVALID;
      total += n * mul;
      v = e + 1;
    }
    r.duration = total;
  }
  return CAL_PARSE_OK;
}
}  // namespace

size_t CalendarIcs_WorkspaceSize() { return sizeof(Work); }
void Calendar_Ascii(const char* in, char* out, size_t cap) {
  if (!cap) return;
  size_t n = 0;
  for (const unsigned char* p = (const unsigned char*)in; *p && n + 1 < cap;) {
    uint32_t c = *p++;
    if (c == '\\' && *p) {
      c = *p++;
      if (c == 'n' || c == 'N') c = ' ';
    } else if (c >= 0xc0) {
      int more = c < 0xe0 ? 1 : c < 0xf0 ? 2 : 3;
      c &= (1 << (6 - more)) - 1;
      while (more-- && (*p & 0xc0) == 0x80) c = (c << 6) | (*p++ & 63);
    }
    if (c < 32) c = ' ';
    if (c < 128) {
      out[n++] = (char)c;
      continue;
    }
    const uint32_t src[] = {0xe1,  0xe4,  0x10d, 0x10f, 0xe9,  0x11b, 0xed,  0x13a, 0x13e,
                            0x148, 0xf3,  0xf4,  0xf6,  0x155, 0x159, 0x161, 0x165, 0xfa,
                            0xfc,  0x16f, 0xfd,  0x17e, 0xc1,  0xc4,  0x10c, 0x10e, 0xc9,
                            0x11a, 0xcd,  0x139, 0x13d, 0x147, 0xd3,  0xd4,  0xd6,  0x154,
                            0x158, 0x160, 0x164, 0xda,  0xdc,  0x16e, 0xdd,  0x17d};
    const char* dst = "aacdeeillnooorrstuuuyzAACDEEILLNOOORRSTUUUYZ";
    bool found = false;
    for (size_t i = 0; i < sizeof(src) / sizeof(*src); i++)
      if (c == src[i]) {
        out[n++] = dst[i];
        found = true;
        break;
      }
    if (!found) out[n++] = '?';
  }
  out[n] = 0;
}
CalendarParseResult CalendarIcs_Parse(const char* data, size_t len, time_t from, time_t until,
                                      uint8_t source, CalendarEvent* out, size_t capacity,
                                      size_t& count, void* workspace, void (*poll)()) {
  count = 0;
  if (!data || !out || !workspace || until <= from) return CAL_PARSE_INVALID;
  auto& w = *(Work*)workspace;
  w.exceptionN = 0;
  w.steps = 0;
  auto emit = [&](const Raw& r, time_t start) -> CalendarParseResult {
    if (!r.hasStart || r.cancelled) return CAL_PARSE_OK;
    long duration = r.hasEnd ? (long)(r.end.epoch - r.start.epoch) : r.duration;
    if (!duration && r.start.wall.all) duration = 86400;
    if (duration < 0 || duration > 366L * 86400 ||
        (r.hasEnd && (r.end.wall.all != r.start.wall.all || r.end.epoch <= r.start.epoch)))
      return CAL_PARSE_INVALID;
    time_t end = start + duration;
    // All-day end dates are exclusive; preserve calendar-day length across DST.
    if (r.start.wall.all) {
      Date d = r.start.wall;
      long days = r.hasEnd ? civil(r.end.wall.y, r.end.wall.m, r.end.wall.d) - civil(d.y, d.m, d.d)
                           : duration / 86400;
      Date e;
      struct tm lt{};
      localtime_r(&start, &lt);
      e = dateFrom(civil(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday) + (days ? days : 1));
      if (!epoch(e, "", end)) return CAL_PARSE_INVALID;
    }
    if (start >= until || (end > start ? end <= from : start < from)) return CAL_PARSE_OK;
    for (int i = 0; i < r.excludedN; i++)
      if (r.excluded[i] == start) return CAL_PARSE_OK;
    if (!r.hasRid)
      for (size_t i = 0; i < w.exceptionN; i++)
        if (r.uid && w.exceptions[i].uid == r.uid && w.exceptions[i].rid == start)
          return CAL_PARSE_OK;
    for (size_t i = 0; i < count; i++)
      if (r.uid && out[i].uidHash == r.uid && out[i].start == start) return CAL_PARSE_OK;
    if (count == capacity) return CAL_PARSE_LIMIT;
    auto& e = out[count++];
    e = {};
    e.start = start;
    e.end = end;
    e.uidHash = r.uid;
    e.source = source;
    e.allDay = r.start.wall.all;
    snprintf(e.title, sizeof(e.title), "%s", *r.title ? r.title : "-");
    snprintf(e.location, sizeof(e.location), "%s", r.location);
    return CAL_PARSE_OK;
  };
  for (int pass = 0; pass < 2; pass++) {
    size_t pos = 0;
    bool inside = false, calendar = false, ended = false;
    int nested = 0;
    bool over;
    while (line(data, len, pos, w.line, over)) {
      if (poll) poll();
      if (!strcmp(w.line, "BEGIN:VCALENDAR")) {
        calendar = true;
        continue;
      }
      if (!strcmp(w.line, "END:VCALENDAR")) {
        ended = true;
        continue;
      }
      if (!strcmp(w.line, "BEGIN:VEVENT")) {
        if (inside) return CAL_PARSE_INVALID;
        inside = true;
        nested = 0;
        w.raw = {};
        continue;
      }
      if (!inside) continue;
      if (!strcmp(w.line, "END:VEVENT")) {
        inside = false;
        auto& r = w.raw;
        if (r.hasRid && pass == 0) {
          if (!r.uid) return CAL_PARSE_INVALID;
          if (r.rid.epoch >= from - 366L * 86400 && r.rid.epoch < until) {
            if (w.exceptionN == EXCEPTIONS) return CAL_PARSE_LIMIT;
            w.exceptions[w.exceptionN++] = {r.uid, r.rid.epoch};
          }
          auto e = emit(r, r.start.epoch);
          if (e != CAL_PARSE_OK) return e;
        }
        if (!r.cancelled && !r.hasStart && !r.hasRid) return CAL_PARSE_INVALID;
        if (!r.hasRid && pass == 1 && !r.cancelled && r.hasStart) {
          if (!*r.rule) {
            auto e = emit(r, r.start.epoch);
            if (e != CAL_PARSE_OK) return e;
          } else {
            Rule rule;
            if (!ruleParse(r, rule)) return CAL_PARSE_RULE;
            long startDay = civil(r.start.wall.y, r.start.wall.m, r.start.wall.d);
            long first = startDay;
            if (!rule.count) {
              struct tm t{};
              // Preserve overlapping long events without scanning old history.
              long duration = r.hasEnd ? r.end.epoch - r.start.epoch : r.duration;
              time_t lower = from - (duration > 0 ? duration : 86400) - 86400;
              gmtime_r(&lower, &t);
              long f = civil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
              if (f > first) first = f;
            }
            int occurrence = 0;
            for (long day = first;; day++) {
              if (++w.steps > 1000000) return CAL_PARSE_LIMIT;
              if (poll && (w.steps % 128 == 0)) poll();
              Date d = dateFrom(day);
              d.h = r.start.wall.h;
              d.min = r.start.wall.min;
              d.s = r.start.wall.s;
              d.all = r.start.wall.all;
              d.utc = r.start.wall.utc;
              time_t start;
              if (!epoch(d, r.start.zone, start)) return CAL_PARSE_TIMEZONE;
              if (start >= until || d.y > 2100 || (rule.until && start > rule.until)) break;
              if (day == startDay || matches(d, day, r, rule)) {
                occurrence++;
                if (rule.count && occurrence > rule.count) break;
                auto e = emit(r, start);
                if (e != CAL_PARSE_OK) return e;
              }
            }
          }
          for (int i = 0; i < r.extraN; i++) {
            auto e = emit(r, r.extra[i]);
            if (e != CAL_PARSE_OK) return e;
          }
        }
        continue;
      }
      if (!strncmp(w.line, "BEGIN:", 6)) {
        nested++;
        continue;
      }
      if (!strncmp(w.line, "END:", 4)) {
        if (nested) nested--;
        continue;
      }
      if (nested) continue;
      char* colon = strchr(w.line, ':');
      if (!colon) continue;
      // A colon inside a quoted TZID is rare, but must not split the property.
      bool quoted = false;
      for (char* c = w.line; *c; c++) {
        if (*c == '"') quoted = !quoted;
        if (*c == ':' && !quoted) {
          colon = c;
          break;
        }
      }
      *colon++ = 0;
      if (over && strncmp(w.line, "DESCRIPTION", 11) && strncmp(w.line, "ATTACH", 6) &&
          strncmp(w.line, "X-", 2))
        return CAL_PARSE_LIMIT;
      auto e = property(w.raw, w.line, colon);
      if (e != CAL_PARSE_OK) return e;
    }
    if (!calendar || !ended || inside) return CAL_PARSE_INVALID;
  }
  return CAL_PARSE_OK;
}
