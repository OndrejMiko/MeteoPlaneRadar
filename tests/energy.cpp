#include "Energy.h"
#include "Settings.h"
#include "Lang.h"
#include "Status.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

static std::vector<std::string> requests;
extern "C" time_t time(time_t* out) {
  const time_t now = 1793275200;
  if (out) *out = now;
  return now;
}
bool Outside_TimeValid() { return true; }
void Status_Set(StatusSlot, const char*, ...) {}
bool Net_GetString(const char* url, String& body, const char*) {
  requests.emplace_back(url);
  if (!strcmp(url, PRICE_URL)) {
    body = R"({"hoursToday":[{"hour":0,"minute":0,"priceCZK":2000},{"hour":0,"minute":15,"priceCZK":-500}],"hoursTomorrow":[{"hour":1,"minute":30,"priceCZK":1500}]})";
  } else {
    body = R"({"unix_seconds":[1793275200],"production_types":[{"name":"Nuclear","data":[1000]},{"name":"Fossil brown coal / lignite","data":[500]},{"name":"Solar","data":[250]},{"name":"New source","data":[250]},{"name":"Load","data":[1800]},{"name":"Hydro pumped storage consumption","data":[-100]}]})";
  }
  return true;
}
int main() {
  Settings_ClearAll(); Settings_Begin();
  Settings_SetLocation(50.08,14.42);
  Settings_SetScreenEnabled(SCREEN_PRICE_I,true);
  Settings_SetScreenEnabled(SCREEN_MIX_I,true);
  // A saved enabled flag is insufficient outside Czech: no HTTP at all.
  for (uint8_t lang : {LANG_SK,LANG_EN}) {
    Settings_SetLanguage(lang); Energy_Invalidate(); Energy_Tick();
    assert(requests.empty());
    assert(!Settings_ScreenEnabled(SCREEN_PRICE_I));
    assert(!Settings_ScreenEnabled(SCREEN_MIX_I));
  }
  Settings_SetLanguage(LANG_CZ);
  Energy_Tick();
  assert(requests.size()==1 && requests[0]==PRICE_URL);
  assert(Energy_PriceValid() && Energy_TomorrowValid());
  assert(Energy_PriceToday(0)==2000 && Energy_PriceToday(1)==-500);
  assert(std::isnan(Energy_PriceToday(2)));
  assert(Energy_CheapestSlot(0)==1 && Energy_PriceHighestSlot(0)==0);
  Settings_SetPriceFee(100); Settings_SetPriceVat(21);
  assert(std::fabs(Energy_FinalCzkKwh(2000)-2.541f)<0.0001f);
  Energy_Tick();
  assert(requests.size()==2 && requests[1].find("country=cz&")!=std::string::npos);
  assert(Energy_MixValid() && Energy_MixTotalMw()==2000);
  assert(Energy_MixMw(MIX_COAL)==500 && Energy_MixMw(MIX_OTHER)==250);
  assert(Energy_MixShare(MIX_NUCLEAR)==50);
  for (uint8_t lang : {LANG_SK,LANG_EN}) {
    Settings_SetLanguage(lang); Energy_Invalidate(); Energy_Tick();
    assert(requests.size()==2); // cached data never causes polling while hidden
  }
  Settings_SetLanguage(LANG_CZ); Settings_SetLocation(40,-74);
  Energy_Invalidate(); Energy_Tick(); assert(requests.size()==2);
  Settings_SetLocation(50.08,14.42);
  Settings_SetScreenEnabled(SCREEN_PRICE_I,false);
  Settings_SetScreenEnabled(SCREEN_MIX_I,false);
  Energy_Tick(); assert(requests.size()==2);
  std::cout << "Energy language gating, no hidden requests, source area, quarter-hour prices and mix parsing passed\n";
}
