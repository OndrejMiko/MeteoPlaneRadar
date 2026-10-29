#include "CalendarIcs.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

time_t utc(int y, int m, int d, int h = 0, int min = 0) {
  tm t{};
  t.tm_year = y - 1900;
  t.tm_mon = m - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = min;
  return timegm(&t);
}
std::string event(const std::string& body) { return "BEGIN:VEVENT\r\n" + body + "END:VEVENT\r\n"; }
std::string eastern(const std::string& id = "America/New_York") {
  return "BEGIN:VTIMEZONE\r\nTZID:" + id +
         "\r\nBEGIN:STANDARD\r\nDTSTART:20071104T020000\r\nTZOFFSETFROM:-0400\r\nTZOFFSETTO:-"
         "0500\r\nRRULE:FREQ=YEARLY;BYMONTH=11;BYDAY=1SU\r\nEND:STANDARD\r\nBEGIN:"
         "DAYLIGHT\r\nDTSTART:20070311T020000\r\nTZOFFSETFROM:-0500\r\nTZOFFSETTO:-0400\r\nRRULE:"
         "FREQ=YEARLY;BYMONTH=3;BYDAY=2SU\r\nEND:DAYLIGHT\r\nEND:VTIMEZONE\r\n";
}
int main() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  std::vector<unsigned char> work(CalendarIcs_WorkspaceSize());
  CalendarEvent out[128];
  size_t count = 0;
  CalendarParseDiagnostics diagnostics{};
  auto parse = [&](std::string body, time_t from = utc(2026, 10, 1),
                   time_t until = utc(2026, 12, 1)) {
    std::string feed = "BEGIN:VCALENDAR\r\nVERSION:2.0\r\n" + body + "END:VCALENDAR\r\n";
    return CalendarIcs_Parse(feed.data(), feed.size(), from, until, 0, out, 128, count, work.data(),
                             nullptr, &diagnostics);
  };
  for (const char* prefix : {"/freeassociation.sourceforge.net/Tzfile/",
                             "/var/db/timezone/zoneinfo/", "/mozilla.org/20070129_1/", "/"}) {
    std::string zone = std::string(prefix) + "Europe/Bratislava";
    assert(parse(event("DTSTART;TZID=\"" + zone + "\":20261024T090000\r\n")) == CAL_PARSE_OK);
    assert(count == 1 && out[0].start == utc(2026, 10, 24, 7));
  }
  assert(parse(event("DTSTART;TZID=Unrecognised/Europe/Prague:20261024T090000\r\n")) ==
         CAL_PARSE_TIMEZONE);
  assert(!strcmp(diagnostics.timezone, "Unrecognised/Europe/Prague"));
  auto recurring = event(
      "UID:work\r\nDTSTART;TZID=America/New_York:20261026T090000\r\nDTEND;TZID=America/"
      "New_York:20261026T100000\r\nRRULE:FREQ=WEEKLY;BYDAY=MO;COUNT=2\r\n");
  for (auto body : {eastern() + recurring, recurring + eastern()}) {
    assert(parse(body) == CAL_PARSE_OK && count == 2);
    assert(out[0].start == utc(2026, 10, 26, 13) && out[1].start == utc(2026, 11, 2, 14));
  }
  // Ambiguous fall hour means its first occurrence, not standard time.
  assert(parse(eastern() + event("DTSTART;TZID=America/New_York:20261101T013000\r\n")) ==
         CAL_PARSE_OK);
  assert(out[0].start == utc(2026, 11, 1, 5, 30));
  // Missing spring hour uses the offset before the jump, per RFC 5545.
  assert(parse(eastern() + event("DTSTART;TZID=America/New_York:20260308T023000\r\n"),
               utc(2026, 3, 1), utc(2026, 4, 1)) == CAL_PARSE_OK);
  assert(out[0].start == utc(2026, 3, 8, 7, 30));
  std::string half =
      "BEGIN:VTIMEZONE\r\nTZID:Custom/"
      "Nepal\r\nBEGIN:STANDARD\r\nDTSTART:19700101T000000\r\nTZOFFSETFROM:+0545\r\nTZOFFSETTO:+"
      "0545\r\nEND:STANDARD\r\nEND:VTIMEZONE\r\n";
  assert(parse(half + event("DTSTART;TZID=Custom/Nepal:20261024T100000\r\n")) == CAL_PARSE_OK);
  assert(out[0].start == utc(2026, 10, 24, 4, 15));
  std::string south =
      "BEGIN:VTIMEZONE\r\nTZID:Custom/"
      "Sydney\r\nBEGIN:DAYLIGHT\r\nDTSTART:20001001T020000\r\nTZOFFSETFROM:+1000\r\nTZOFFSETTO:+"
      "1100\r\nRRULE:FREQ=YEARLY;BYMONTH=10;BYDAY=1SU\r\nEND:DAYLIGHT\r\nBEGIN:STANDARD\r\nDTSTART:"
      "20010401T030000\r\nTZOFFSETFROM:+1100\r\nTZOFFSETTO:+1000\r\nRRULE:FREQ=YEARLY;BYMONTH=4;"
      "BYDAY=1SU\r\nEND:STANDARD\r\nEND:VTIMEZONE\r\n";
  assert(parse(south + event("DTSTART;TZID=Custom/Sydney:20261002T120000\r\n") +
               event("DTSTART;TZID=Custom/Sydney:20261005T120000\r\n")) == CAL_PARSE_OK);
  assert(out[0].start == utc(2026, 10, 2, 2) && out[1].start == utc(2026, 10, 5, 1));
  // Unsupported rules remain explicit errors; valid UTC events need no definition.
  auto bad = eastern();
  bad.replace(bad.find("BYDAY=1SU"), 9, "BYSETPOS=1");
  assert(parse(bad + event("DTSTART;TZID=America/New_York:20261024T090000\r\n")) ==
         CAL_PARSE_TIMEZONE);
  assert(!strcmp(diagnostics.timezone, "America/New_York"));
  assert(parse(bad + event("DTSTART:20261024T090000Z\r\n")) == CAL_PARSE_OK);
  // Definitions are per parse: a previous source must never resolve a new feed.
  assert(parse(event("DTSTART;TZID=America/New_York:20261024T090000\r\n")) == CAL_PARSE_TIMEZONE);
  // Explicit RDATE transitions, without a recurrence rule.
  auto dates = half;
  dates.insert(dates.find("END:STANDARD"), "RDATE:20261025T020000\r\n");
  assert(parse(dates + event("DTSTART;TZID=Custom/Nepal:20261026T100000\r\n")) == CAL_PARSE_OK);
  assert(out[0].start == utc(2026, 10, 26, 4, 15));
  auto invalid = half;
  invalid.replace(invalid.find("+0545"), 5, "+2560");
  assert(parse(invalid + event("DTSTART;TZID=Custom/Nepal:20261024T100000\r\n")) ==
         CAL_PARSE_TIMEZONE);
  std::string many;
  for (int i = 0; i < 9; ++i) many += eastern("Zone/" + std::to_string(i));
  assert(parse(many) == CAL_PARSE_LIMIT);
  auto parametrised = dates;
  parametrised.replace(parametrised.find("RDATE:"), 6, "RDATE;VALUE=DATE-TIME:");
  assert(parse(parametrised + event("DTSTART;TZID=Custom/Nepal:20261026T100000\r\n")) ==
         CAL_PARSE_OK);
  assert(out[0].start == utc(2026, 10, 26, 4, 15));
  // An explicit valid definition takes precedence over a familiar identifier.
  auto named = half;
  named.replace(named.find("Custom/Nepal"), 12, "Europe/Prague");
  assert(parse(named + event("DTSTART;TZID=Europe/Prague:20261024T100000\r\n")) == CAL_PARSE_OK);
  assert(out[0].start == utc(2026, 10, 24, 4, 15));
  std::string explicitDates =
      "BEGIN:VTIMEZONE\r\nTZID:Custom/Dates\r\n"
      "BEGIN:DAYLIGHT\r\nDTSTART:20260329T020000\r\nRDATE:20270328T020000\r\n"
      "TZOFFSETFROM:+0100\r\nTZOFFSETTO:+0200\r\nEND:DAYLIGHT\r\n"
      "BEGIN:STANDARD\r\nDTSTART:20261025T030000\r\nRDATE:20271031T030000\r\n"
      "TZOFFSETFROM:+0200\r\nTZOFFSETTO:+0100\r\nEND:STANDARD\r\nEND:VTIMEZONE\r\n";
  assert(parse(explicitDates + event("DTSTART;TZID=Custom/Dates:20270327T090000\r\n") +
                   event("DTSTART;TZID=Custom/Dates:20270329T090000\r\n"),
               utc(2027, 3, 1), utc(2027, 4, 1)) == CAL_PARSE_OK);
  assert(out[0].start == utc(2027, 3, 27, 8) && out[1].start == utc(2027, 3, 29, 7));
  // Synthetic reproduction of iCloud's full history: pre-1900 LMT offsets
  // used to invalidate an otherwise supported modern Pacific definition.
  auto historical = eastern("Custom/Pacific");
  size_t pos = 0;
  while ((pos = historical.find("-0400", pos)) != std::string::npos) {
    historical.replace(pos, 5, "-0700");
    pos += 5;
  }
  pos = 0;
  while ((pos = historical.find("-0500", pos)) != std::string::npos) {
    historical.replace(pos, 5, "-0800");
    pos += 5;
  }
  historical.insert(historical.find("BEGIN:STANDARD"),
                    "BEGIN:STANDARD\r\nDTSTART:18500101T000000\r\nRDATE:18500101T000000\r\n"
                    "TZOFFSETFROM:-075258\r\nTZOFFSETTO:-075258\r\nEND:STANDARD\r\n"
                    "BEGIN:STANDARD\r\nDTSTART:18831118T120702\r\nRDATE:18831118T120702\r\n"
                    "TZOFFSETFROM:-075258\r\nTZOFFSETTO:-0800\r\nEND:STANDARD\r\n");
  assert(
      parse(
          historical +
          event("DTSTART;TZID=Custom/Pacific:20261026T090000\r\nRRULE:FREQ=WEEKLY;COUNT=2\r\n")) ==
      CAL_PARSE_OK);
  assert(count == 2 && out[0].start == utc(2026, 10, 26, 16) &&
         out[1].start == utc(2026, 11, 2, 17));
  // Earlier timezone metadata does not expand the supported event date domain.
  assert(parse(event("DTSTART:18831118T120702Z\r\n")) == CAL_PARSE_INVALID);
  auto ancient = half;
  ancient.replace(ancient.find("19700101"), 8, "16010101");
  assert(parse(ancient + event("DTSTART;TZID=Custom/Nepal:20261024T100000\r\n")) == CAL_PARSE_OK);
  assert(out[0].start == utc(2026, 10, 24, 4, 15));
  // Historical UNTIL and generated rule dates use the same metadata-only bound.
  auto historicalRule = ancient;
  historicalRule.insert(historicalRule.find("END:VTIMEZONE"),
                        "BEGIN:STANDARD\r\nDTSTART:18500331T020000\r\n"
                        "RRULE:FREQ=YEARLY;UNTIL=18510330T000000Z;BYMONTH=3;BYDAY=-1SU\r\n"
                        "TZOFFSETFROM:+0545\r\nTZOFFSETTO:+0545\r\nEND:STANDARD\r\n");
  assert(parse(historicalRule + event("DTSTART;TZID=Custom/Nepal:20261024T100000\r\n")) ==
         CAL_PARSE_OK);
  assert(out[0].start == utc(2026, 10, 24, 4, 15));
  assert(CalendarIcs_WorkspaceSize() < 80 * 1024);
  std::cout << "VTIMEZONE aliases, annual DST, hemisphere, gaps/folds, offsets, limits and "
               "isolation passed\n";
}
