#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "CalendarIcs.h"

time_t utc(int y, int m, int d, int h = 0, int min = 0) {
  struct tm t{};
  t.tm_year = y - 1900;
  t.tm_mon = m - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = min;
  return timegm(&t);
}
std::string wrap(const std::string& s) {
  return "BEGIN:VCALENDAR\r\nVERSION:2.0\r\n" + s + "END:VCALENDAR\r\n";
}
std::string event(const std::string& s) { return "BEGIN:VEVENT\r\n" + s + "END:VEVENT\r\n"; }
int main() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  std::vector<unsigned char> work(CalendarIcs_WorkspaceSize());
  CalendarEvent out[128];
  size_t count = 0;
  auto parse = [&](const std::string& s, size_t cap = 128) {
    return CalendarIcs_Parse(s.data(), s.size(), utc(2026, 10, 1), utc(2026, 12, 1), 1, out, cap,
                             count, work.data());
  };
  assert(parse(wrap(event(
             "UID:one\r\nDTSTART:20261024T100000Z\r\nDTEND:20261024T110000Z\r\nSUMMARY:Rodinný "
             "obe\r\n d\\, doma\r\nLOCATION:Bratislava\\nCentrum\r\nBEGIN:VALARM\r\nSUMMARY:Ignore "
             "this alarm\r\nEND:VALARM\r\n"))) == CAL_PARSE_OK);
  assert(count == 1 && out[0].start == utc(2026, 10, 24, 10));
  assert(!strcmp(out[0].title, "Rodinny obed, doma"));
  assert(!strcmp(out[0].location, "Bratislava Centrum"));
  assert(out[0].source == 1);
  const auto master = event(
      "UID:weekly\r\nDTSTART;TZID=Europe/Bratislava:20261019T090000\r\nDTEND;TZID=Europe/"
      "Bratislava:20261019T100000\r\nRRULE:FREQ=WEEKLY;BYDAY=MO,WE;COUNT=4\r\nEXDATE;TZID=Europe/"
      "Bratislava:20261021T090000\r\nSUMMARY:Meeting\r\n");
  const auto moved = event(
      "UID:weekly\r\nRECURRENCE-ID;TZID=Europe/Bratislava:20261026T090000\r\nDTSTART;TZID=Europe/"
      "Bratislava:20261026T140000\r\nDTEND;TZID=Europe/Bratislava:20261026T150000\r\nSUMMARY:Moved "
      "meeting\r\n");
  for (auto body : {master + moved, moved + master}) {
    assert(parse(wrap(body)) == CAL_PARSE_OK && count == 3);
    bool old = false, newTime = false, dst = false;
    for (size_t i = 0; i < count; i++) {
      old |= out[i].start == utc(2026, 10, 26, 8);
      newTime |= out[i].start == utc(2026, 10, 26, 13);
      dst |= out[i].start == utc(2026, 10, 28, 8);
    }
    assert(!old && newTime && dst);
  }
  assert(parse(wrap(master + event("UID:weekly\r\nRECURRENCE-ID;TZID=Europe/"
                                   "Bratislava:20261026T090000\r\nSTATUS:CANCELLED\r\n"))) ==
             CAL_PARSE_OK &&
         count == 2);
  assert(parse(wrap(event("UID:daily\r\nDTSTART;VALUE=DATE:20261024\r\nDTEND;VALUE=DATE:"
                          "20261026\r\nRRULE:FREQ=DAILY;COUNT=3\r\nSUMMARY:Trip\r\n"))) ==
             CAL_PARSE_OK &&
         count == 3);
  // A 2-day all-day event spanning the autumn DST transition lasts 49 hours.
  assert(out[0].allDay && out[0].end - out[0].start == 49 * 3600);
  assert(out[1].end - out[1].start == 49 * 3600 && out[2].end - out[2].start == 48 * 3600);
  assert(parse(wrap(event("UID:monthly\r\nDTSTART:20261002T100000Z\r\nRRULE:FREQ=MONTHLY;BYDAY=1FR;"
                          "COUNT=2\r\nSUMMARY:First Friday\r\n"))) == CAL_PARSE_OK &&
         count == 2);
  assert(out[1].start == utc(2026, 11, 6, 10));
  assert(parse(wrap(event("UID:last\r\nDTSTART:20261030T100000Z\r\nRRULE:FREQ=MONTHLY;BYMONTHDAY=-"
                          "1;UNTIL=20261130T100000Z\r\nSUMMARY:Last day\r\n"))) == CAL_PARSE_OK &&
         count == 3);  // DTSTART plus Oct 31 and Nov 30
  assert(parse(wrap(event("UID:rdate\r\nDTSTART:20261024T100000Z\r\nRDATE:20261024T100000Z,"
                          "20261025T100000Z\r\nSUMMARY:Extra\r\n"))) == CAL_PARSE_OK &&
         count == 2);
  assert(parse(wrap(event("UID:a\r\nDTSTART:20261024T100000Z\r\nSUMMARY:Same\r\n") +
                    event("UID:b\r\nDTSTART:20261024T100000Z\r\nSUMMARY:Same\r\n"))) ==
             CAL_PARSE_OK &&
         count == 2);
  assert(parse(wrap(event("DTSTART:20261024T100000Z\r\nDURATION:PT1H30M\r\n"))) == CAL_PARSE_OK &&
         out[0].end - out[0].start == 5400);
  assert(parse(wrap(event("DTSTART;TZID=Unknown/Zone:20261024T100000\r\n"))) == CAL_PARSE_TIMEZONE);
  assert(parse(wrap(
             event("DTSTART:20261024T100000Z\r\nRRULE:FREQ=MONTHLY;BYSETPOS=1;BYDAY=MO,TU\r\n"))) ==
         CAL_PARSE_RULE);
  assert(parse(wrap(event("DTSTART:20261024T100000Z\r\nRRULE:FREQ=DAILY\r\n")), 1) ==
         CAL_PARSE_LIMIT);
  for (const char* invalid :
       {"20261301T100000Z", "20261032T100000Z", "20261024T250000Z", "20261024X100000Z"})
    assert(parse(wrap(event(std::string("DTSTART:") + invalid + "\r\n"))) == CAL_PARSE_INVALID);
  assert(parse("<html>Not a calendar</html>") == CAL_PARSE_INVALID);
  assert(parse("BEGIN:VCALENDAR\nBEGIN:VEVENT\nDTSTART:20261024T100000Z\n") == CAL_PARSE_INVALID);
  assert(parse(wrap("")) == CAL_PARSE_OK && count == 0);
  assert(parse(wrap(event("UID:birthday\r\nDTSTART;VALUE=DATE:19501024\r\nRRULE:FREQ="
                          "YEARLY\r\nSUMMARY:Birthday\r\n"))) == CAL_PARSE_OK &&
         count == 1 && out[0].allDay);
  assert(parse(wrap(event("SUMMARY:Missing date\r\n"))) == CAL_PARSE_INVALID);
  char ascii[128];
  Calendar_Ascii("áäčďéěíĺľňóôöŕřšťúüůýžÁÄČĎÉĚÍĹĽŇÓÔÖŔŘŠŤÚÜŮÝŽ", ascii, sizeof(ascii));
  assert(!strcmp(ascii, "aacdeeillnooorrstuuuyzAACDEEILLNOOORRSTUUUYZ"));
  std::cout
      << "ICS parsing, recurrence exceptions, time zones, DST, limits and text tests passed\n";
}
