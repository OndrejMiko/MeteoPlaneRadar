#include "Settings.h"
#include "Lang.h"
#include <Preferences.h>
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  Settings_ClearAll();
  Settings_SetLanguage(LANG_SK);
  Settings_Begin();
  assert(Settings_Language() == LANG_SK && Lang_Get() == LANG_SK);
  assert(!strcmp(T(S_FORECAST), "Predpoved"));
  assert(!strcmp(TW(S_FORECAST), "Predpoveď"));
  assert(!strcmp(T(S_ALDER), "jelsa"));
  assert(!strcmp(Lang_MonthName(9), "oktobra"));
  assert(!strcmp(Lang_WeekdayShort(5), "Pi"));
  for (uint8_t lang = LANG_CZ; lang <= LANG_SK; ++lang) {
    Lang_Set(lang);
    for (int i = 0; i < STR_COUNT; ++i) {
      assert(*T((StrId)i) && *TW((StrId)i));
      for (const unsigned char* c = (const unsigned char*)T((StrId)i); *c; ++c)
        assert(*c < 128);
    }
  }
  assert(!*T(STR_COUNT));
  assert(!*Lang_WeekdayShort(-1) && !*Lang_MonthName(12));
  Settings_SetLanguage(255);
  assert(Settings_Language() == LANG_CZ);

  // Updating a device that only enabled the removed screens must leave one
  // usable data screen. Stored screen indices outside the new set are safe.
  for (int oldScreen = 4; oldScreen <= 6; ++oldScreen) {
    Settings_ClearAll();
    Preferences::numbers["scrM"] = 0x30;
    Preferences::numbers["scr"] = oldScreen;
    Settings_Begin();
    assert(Settings_EnabledCount() == 1);
    assert(Settings_ScreenEnabled(SCREEN_CLOCK_I));
    assert(Settings_Screen() == SCREEN_SETTINGS_I);
    Settings_SetScreenEnabled(SCREEN_CLOCK_I, false);
    assert(Settings_ScreenEnabled(SCREEN_CLOCK_I));
  }
  Settings_ClearAll();
  Preferences::numbers["scrM"] = 0x3a;
  Settings_Begin();
  assert(Settings_EnabledCount() == 2);
  assert(Settings_ScreenEnabled(SCREEN_PLANES_I));
  assert(Settings_ScreenEnabled(SCREEN_FORECAST_I));
  assert(!Settings_ScreenEnabled(SCREEN_N));

  // An older backup imports the remaining settings and cannot restore either
  // removed feature. Export contains only the four remaining data screens.
  JsonDocument backup;
  deserializeJson(backup, R"({"lang":2,"priceFee":100,"priceVat":20,"mixCountry":"sk","screens":{"clock":true,"price":true,"mix":true}})");
  Settings_FromJson(backup.as<JsonObjectConst>());
  Settings_Begin();
  assert(Settings_Language() == LANG_SK);
  JsonDocument exported;
  Settings_ToJson(exported.to<JsonObject>());
  assert(exported["lang"] == LANG_SK);
  assert(exported["screens"].as<JsonObjectConst>().size() == 5);
  assert(exported["priceFee"].isNull() && exported["priceVat"].isNull());
  assert(exported["mixCountry"].isNull());
  assert(exported["screens"]["price"].isNull() && exported["screens"]["mix"].isNull());
  // Saved links are write-only, survive reboot, normalize webcal and are not
  // overwritten when a browser submits blank fields after loading config.
  JsonDocument cal;
  deserializeJson(cal,R"({"calendarView":1,"calendarRefresh":10,"calendars":[{"name":"Rodina","url":"webcal://example.com/private.ics","color":123,"enabled":true}]})");
  assert(!Settings_ValidateCalendarJson(cal.as<JsonObjectConst>()));
  Settings_FromJson(cal.as<JsonObjectConst>());
  Settings_Begin();
  assert(Settings_CalendarCount()==1 && Settings_CalendarView()==1);
  assert(Settings_CalendarRefresh()==10);
  assert(!strcmp(Settings_CalendarSource(0).url,"https://example.com/private.ics"));
  cal["calendars"][0]["url"]="";
  Settings_FromJson(cal.as<JsonObjectConst>());
  assert(*Settings_CalendarSource(0).url);
  exported.clear();Settings_ToJson(exported.to<JsonObject>());
  assert(exported["calendars"][0]["url"].isNull());
  assert(exported["calendars"][0]["hasUrl"]==true);
  std::string json;serializeJson(exported,json);assert(json.find("private.ics")==std::string::npos);
  cal["calendars"][0]["url"]="http://example.com/private.ics";
  assert(Settings_ValidateCalendarJson(cal.as<JsonObjectConst>()));
  assert(!Settings_FromJson(cal.as<JsonObjectConst>()));
  assert(!strcmp(Settings_CalendarSource(0).url,"https://example.com/private.ics"));
  cal["calendars"][0]["url"]="";cal["calendars"][0]["clearUrl"]=true;
  Settings_FromJson(cal.as<JsonObjectConst>());assert(Settings_CalendarCount()==0);
  cal["calendarRefresh"]=0;assert(Settings_ValidateCalendarJson(cal.as<JsonObjectConst>()));
  std::cout << "Settings migration, backup and language tests passed\n";
}
